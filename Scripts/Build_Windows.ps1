param(
    [string]$ProjectRoot = (Get-Location).Path,
    [string]$Configuration = "Development",
    [string]$Platform = "Win64",
    [string]$Target = "NewWorldOrderEditor",
    [switch]$RestartEditor,
    [string]$Map = "",
    [switch]$WaitForReady,
    [int]$ReadyTimeoutSec = 180
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-EngineDirFromRegistry {
    param([string]$EngineAssoc)
    $buildsHkcu = Get-ItemProperty "HKCU:\Software\Epic Games\Unreal Engine\Builds" -ErrorAction SilentlyContinue
    $buildsHklm = Get-ItemProperty "HKLM:\Software\Epic Games\Unreal Engine\Builds" -ErrorAction SilentlyContinue

    function Get-RegisteredEnginePath {
        param($RegistryObject, [string]$Association)
        if (-not $RegistryObject) {
            return $null
        }

        $property = $RegistryObject.PSObject.Properties[$Association]
        if ($property) {
            return $property.Value
        }

        return $null
    }

    # EngineAssociation 可能是 "5.7" 这种带点的名字，StrictMode 下不能直接用对象属性访问。
    $candidate = @(
        (Get-RegisteredEnginePath -RegistryObject $buildsHkcu -Association $EngineAssoc),
        (Get-RegisteredEnginePath -RegistryObject $buildsHklm -Association $EngineAssoc)
    ) | Where-Object { $_ } | Select-Object -First 1
    return $candidate
}

function Find-EngineDirByScan {
    param([string]$EngineAssoc)
    $roots = @(
        "$env:ProgramFiles\Epic Games",
        "$env:ProgramFiles(x86)\Epic Games",
        "C:\Program Files\Epic Games",
        "D:\Program Files\Epic Games",
        "E:\Program Files\Epic Games",
        "F:\Program Files\Epic Games",
        "G:\Program Files\Epic Games"
    ) | Select-Object -Unique

    foreach ($root in $roots) {
        if (-not $root) { continue }
        if (-not (Test-Path $root)) { continue }
        $candidate = Join-Path $root ("UE_{0}" -f $EngineAssoc)
        if (Test-Path $candidate) {
            return $candidate
        }
    }
    return $null
}

$uprojectPath = Join-Path $ProjectRoot "NewWorldOrder.uproject"
if (-not (Test-Path $uprojectPath)) {
    throw "Project not found: $uprojectPath"
}
$uprojectPath = (Resolve-Path -LiteralPath $uprojectPath).Path

$uproject = Get-Content $uprojectPath | ConvertFrom-Json
$engineAssoc = $uproject.EngineAssociation
if (-not $engineAssoc) {
    throw "EngineAssociation not found in uproject."
}

$engineDir = $env:UE_ENGINE_DIR
if (-not $engineDir) {
    $engineDir = Get-EngineDirFromRegistry -EngineAssoc $engineAssoc
}
if (-not $engineDir) {
    $engineDir = Find-EngineDirByScan -EngineAssoc $engineAssoc
}
if (-not $engineDir -or -not (Test-Path $engineDir)) {
    throw "EngineDirNotFound. Set UE_ENGINE_DIR or register EngineAssociation in Unreal Engine Builds."
}

$buildBat = Join-Path $engineDir "Engine\Build\BatchFiles\Build.bat"
if (-not (Test-Path $buildBat)) {
    throw "Build.bat not found: $buildBat"
}
$editorExe = Join-Path $engineDir "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $editorExe)) {
    throw "UnrealEditor.exe not found: $editorExe"
}

function Get-ProjectEditorProcesses {
    param([string]$ProjectFile)

    $normalizedProjectFile = $ProjectFile.Replace('/', '\')
    $escapedProjectFile = [Regex]::Escape($normalizedProjectFile)
    $projectArgumentPattern = "(?i)(?:^|\s)`"?$escapedProjectFile`"?(?:\s|$)"

    # 只按完整 .uproject 参数识别本项目。禁止按 UnrealEditor.exe 名称关闭进程，
    # 否则会误关同时运行的 Lyra 或其他 Unreal 项目。
    return @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" | Where-Object {
        $_.CommandLine -and $_.CommandLine.Replace('/', '\') -match $projectArgumentPattern
    })
}

function Stop-ProjectEditorProcesses {
    param([object[]]$Processes)

    foreach ($processInfo in $Processes) {
        $process = Get-Process -Id $processInfo.ProcessId -ErrorAction SilentlyContinue
        if ($process -and $process.MainWindowHandle -ne 0) {
            Write-Host "Closing NewWorldOrder editor PID=$($process.Id) ..."
            $process.CloseMainWindow() | Out-Null
        }
    }

    $deadline = (Get-Date).AddSeconds(15)
    do {
        $remaining = @($Processes | Where-Object {
            Get-Process -Id $_.ProcessId -ErrorAction SilentlyContinue
        })
        if ($remaining.Count -eq 0) {
            return
        }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)

    foreach ($processInfo in $remaining) {
        Write-Host "NewWorldOrder editor PID=$($processInfo.ProcessId) did not exit; terminating that process tree only."
        & taskkill.exe /F /PID $processInfo.ProcessId /T | Out-Null
    }
}

$projectEditors = @(Get-ProjectEditorProcesses -ProjectFile $uprojectPath)
if ($projectEditors.Count -gt 0) {
    if (-not $RestartEditor) {
        throw "NewWorldOrder editor is running. Close it first, or use -RestartEditor for an isolated cold build and relaunch."
    }
    Stop-ProjectEditorProcesses -Processes $projectEditors
}

Write-Host "EngineAssociation=$engineAssoc"
Write-Host "EngineDir=$engineDir"
Write-Host "Project=$uprojectPath"

# 同一引擎进程仍可能运行 Lyra 参考项目。显式禁用 IDE Hot Reload，确保生成基础模块 DLL，
# 否则新增 UCLASS 只会落到 -000N.dll，NewWorldOrder 重启后无法反射到新类型。
& $buildBat $Target $Platform $Configuration -Project="$uprojectPath" -WaitMutex -NoHotReloadFromIDE
$buildExitCode = $LASTEXITCODE
if ($buildExitCode -ne 0) {
    exit $buildExitCode
}

if ($RestartEditor) {
    # Start-Process 会重新组装 ArgumentList；项目路径含空格，必须保留为带引号的单一参数。
    $editorArguments = '"' + $uprojectPath + '"'
    if ($Map) {
        $editorArguments += ' "' + $Map + '"'
    }

    $editorProcess = Start-Process -FilePath $editorExe -ArgumentList $editorArguments -PassThru
    Write-Host "Started NewWorldOrder editor PID=$($editorProcess.Id)"

    if ($WaitForReady) {
        $signalsDir = Join-Path (Split-Path -Parent $uprojectPath) "Saved\VibeUE\Signals"
        $readySignal = Join-Path $signalsDir "editor-$($editorProcess.Id)-true.json"
        $deadline = (Get-Date).AddSeconds($ReadyTimeoutSec)
        while (-not (Test-Path -LiteralPath $readySignal)) {
            if ($editorProcess.HasExited) {
                throw "NewWorldOrder editor exited before VibeUE became ready."
            }
            if ((Get-Date) -ge $deadline) {
                throw "VibeUE readiness signal timed out after $ReadyTimeoutSec seconds."
            }
            Start-Sleep -Seconds 1
        }
        Write-Host "VibeUE ready: $readySignal"
    }
}

exit 0
