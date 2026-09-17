param(
    [string]$McpUrl = "http://127.0.0.1:8000/mcp",
    [string]$AnimationFolder = "/Game/Characters/Heroes/CC/MF/Animations/Poses/Girl",
    [string]$PoseLibrary = "/Game/Characters/Heroes/CC/Animations/Shared/Poses/DA_PoseLibrary",
    [string]$IconContentFolder = "/Game/Characters/Heroes/CC/Review/Animations/PoseIcons",
    [string]$TempOutputDir = "Saved/PoseIconSource",
    [switch]$SkipCapture,
    [switch]$SkipImport
)

$ErrorActionPreference = "Stop"

function Convert-ContentPathToDiskPath {
    param([string]$ContentPath)

    if (-not $ContentPath.StartsWith("/Game/")) {
        throw "Only /Game content paths are supported: $ContentPath"
    }

    $relativePath = $ContentPath.Substring("/Game/".Length).Replace("/", [IO.Path]::DirectorySeparatorChar)
    return Join-Path (Join-Path (Get-Location) "Content") $relativePath
}

function Get-McpSession {
    $body = @{
        jsonrpc = "2.0"
        id = 1
        method = "initialize"
        params = @{
            protocolVersion = "2024-11-05"
            capabilities = @{}
            clientInfo = @{
                name = "pose-library-icon-generator"
                version = "1.0"
            }
        }
    } | ConvertTo-Json -Depth 10

    $response = Invoke-WebRequest -Uri $McpUrl -Method Post -Body $body -ContentType "application/json"
    $sessionId = $response.Headers["Mcp-Session-Id"]
    if (-not $sessionId) {
        throw "MCP did not return Mcp-Session-Id. Is the Unreal editor MCP server running?"
    }

    return $sessionId
}

function Invoke-McpRequest {
    param(
        [string]$SessionId,
        [hashtable]$Body
    )

    $jsonBody = $Body | ConvertTo-Json -Depth 30
    $response = Invoke-WebRequest -Uri $McpUrl -Method Post -Body $jsonBody -ContentType "application/json" -Headers @{
        "Mcp-Session-Id" = $SessionId
    }

    $dataLine = ($response.Content -split "`n" | Where-Object { $_ -like "data: *" } | Select-Object -First 1)
    if (-not $dataLine) {
        throw "MCP response did not contain an SSE data line: $($response.Content)"
    }

    return ($dataLine.Substring(6) | ConvertFrom-Json)
}

function Invoke-UnrealPython {
    param(
        [string]$SessionId,
        [string]$Code,
        [int]$Id
    )

    $body = @{
        jsonrpc = "2.0"
        id = $Id
        method = "tools/call"
        params = @{
            name = "execute_python_code"
            arguments = @{
                code = $Code
            }
        }
    }

    $response = Invoke-McpRequest -SessionId $SessionId -Body $body
    $contentText = $response.result.content[0].text
    $payload = $contentText | ConvertFrom-Json
    if (-not $payload.success) {
        throw "Unreal Python failed: $($payload.error_message)"
    }
    return $payload.output
}

function Invoke-CaptureAssetImage {
    param(
        [string]$SessionId,
        [string]$AssetPath,
        [int]$Id
    )

    $body = @{
        jsonrpc = "2.0"
        id = $Id
        method = "tools/call"
        params = @{
            name = "call_tool"
            arguments = @{
                toolset_name = "EditorToolset.EditorAppToolset"
                tool_name = "CaptureAssetImage"
                arguments = @{
                    assetPath = $AssetPath
                }
            }
        }
    }

    $response = Invoke-McpRequest -SessionId $SessionId -Body $body
    $contentText = $response.result.content[0].text
    $payload = $contentText | ConvertFrom-Json
    if (-not $payload.returnValue.data) {
        throw "CaptureAssetImage returned no PNG data for $AssetPath"
    }
    return $payload.returnValue.data
}

$animationDiskFolder = Convert-ContentPathToDiskPath -ContentPath $AnimationFolder
if (-not (Test-Path -LiteralPath $animationDiskFolder)) {
    throw "Animation disk folder not found: $animationDiskFolder"
}

$outputDir = Join-Path (Get-Location) $TempOutputDir
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$animationNames = Get-ChildItem -LiteralPath $animationDiskFolder -Filter "*.uasset" |
    Where-Object { $_.Name -notlike "*_BuiltData.uasset" } |
    Sort-Object Name |
    ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_.Name) }

if ($animationNames.Count -eq 0) {
    throw "No animation uassets found under $animationDiskFolder"
}

$sessionId = Get-McpSession
$requestId = 10

if (-not $SkipCapture) {
    foreach ($animationName in $animationNames) {
        $assetPath = "$AnimationFolder/$animationName"
        $pngPath = Join-Path $outputDir "$animationName.png"
        Write-Host "Capture $assetPath -> $pngPath"
        $base64Data = Invoke-CaptureAssetImage -SessionId $sessionId -AssetPath $assetPath -Id $requestId
        [IO.File]::WriteAllBytes($pngPath, [Convert]::FromBase64String($base64Data))
        $requestId++
        Start-Sleep -Milliseconds 300
    }
}

if (-not $SkipImport) {
    $pngPaths = Get-ChildItem -LiteralPath $outputDir -Filter "*.png" | Sort-Object Name | ForEach-Object { $_.FullName }
    $chunks = @()
    for ($i = 0; $i -lt $pngPaths.Count; $i += 8) {
        $chunks += ,($pngPaths[$i..([Math]::Min($i + 7, $pngPaths.Count - 1))])
    }

    foreach ($chunk in $chunks) {
        $chunkJson = $chunk | ConvertTo-Json -Compress
        $iconFolderJson = $IconContentFolder | ConvertTo-Json -Compress
        $code = @"
import unreal, json, os
png_paths = json.loads(r'''$chunkJson''')
icon_folder = json.loads(r'''$iconFolderJson''')
unreal.EditorAssetLibrary.make_directory(icon_folder)
for png_path in png_paths:
    name = os.path.splitext(os.path.basename(png_path))[0]
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', png_path)
    task.set_editor_property('destination_path', icon_folder)
    task.set_editor_property('destination_name', 'T_PoseIcon_' + name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    factory = unreal.TextureFactory()
    task.set_editor_property('factory', factory)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(icon_folder + '/T_PoseIcon_' + name)
    if texture:
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
print('imported', len(png_paths))
"@
        Write-Host "Import icon chunk: $($chunk.Count) PNG files"
        Invoke-UnrealPython -SessionId $sessionId -Code $code -Id $requestId | Write-Host
        $requestId++
        Start-Sleep -Milliseconds 300
    }

    $poseLibraryJson = $PoseLibrary | ConvertTo-Json -Compress
    $iconFolderJson = $IconContentFolder | ConvertTo-Json -Compress
    $code = @"
import unreal, json
pose_library_path = json.loads(r'''$poseLibraryJson''')
icon_folder = json.loads(r'''$iconFolderJson''')
pose_library = unreal.load_asset(pose_library_path)
if not pose_library:
    raise RuntimeError('Pose library not found: ' + pose_library_path)
entries = list(pose_library.get_editor_property('PoseEntries'))
assigned = 0
missing = []
for index, entry in enumerate(entries):
    animation = entry.animation
    if not animation:
        missing.append('entry_%d_no_animation' % index)
        continue
    texture = unreal.load_asset(icon_folder + '/T_PoseIcon_' + animation.get_name())
    if not texture:
        missing.append(animation.get_name())
        continue
    entry.icon = texture
    entries[index] = entry
    assigned += 1
pose_library.set_editor_property('PoseEntries', entries)
unreal.EditorAssetLibrary.save_loaded_asset(pose_library, only_if_is_dirty=False)
unreal.EditorAssetLibrary.save_directory(icon_folder, only_if_is_dirty=False, recursive=True)
print('assigned', assigned)
print('missing', missing)
"@
    Write-Host "Assign imported textures to $PoseLibrary"
    Invoke-UnrealPython -SessionId $sessionId -Code $code -Id $requestId | Write-Host
}

Write-Host "Pose icon generation finished."
