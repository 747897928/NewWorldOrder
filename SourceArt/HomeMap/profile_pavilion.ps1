param([ValidateSet(0,1,2)][int]$ScalabilityTier=2,[switch]$CaptureReview,[string[]]$ReviewLabels=@('HM_Review_Courtyard','HM_Review_Gym'))
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$projectFile = (Resolve-Path -LiteralPath (Join-Path $projectRoot 'NewWorldOrder.uproject')).Path
$editorProcess = Get-CimInstance Win32_Process -Filter "name='UnrealEditor.exe'" | Where-Object { $_.CommandLine -like ('*' + $projectFile + '*') -and $_.CommandLine -notmatch '\s-game\b' } | Select-Object -First 1
if (-not $editorProcess) { throw '未找到当前项目编辑器，不能猜测引擎启动路径。' }
$running = Get-CimInstance Win32_Process -Filter "name='UnrealEditor.exe'" | Where-Object { $_.CommandLine -like '*HomeMap_Pavilion_Profile*' }
if ($running) { throw '已有本轮采样进程，先检查该进程，不重复启动。' }
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$qualityName = @('LegacyLow','Medium','High')[$ScalabilityTier]
$logPath = Join-Path $projectRoot ('Saved/Logs/HomeMap_Pavilion_Profile_' + $qualityName + '_' + $stamp + '.log')
$cameras = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'review_cameras.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$captureFrames = 600 + $cameras.Count * 1200
$quality = 'sg.ViewDistanceQuality 2,sg.AntiAliasingQuality 2,sg.ShadowQuality 2,sg.GlobalIlluminationQuality 2,sg.ReflectionQuality 2,sg.PostProcessQuality 2,sg.TextureQuality 2,sg.EffectsQuality 2,sg.FoliageQuality 2,r.SetRes 1920x1080w,r.ScreenPercentage 100,t.MaxFPS 0,csvprofile frames=' + $captureFrames
$quality = $quality.Replace('Quality 2',('Quality '+$ScalabilityTier))
# UE 的材质质量枚举不是 High/Medium/Low 的直觉顺序：0=Low、1=High、2=Medium。
# 显式设置它，确保 M_Mirror 的 Quality Switch 在本次独立进程中实际走对应分支。
$materialQualityLevel = @(0,2,1)[$ScalabilityTier]
$quality += ',r.MaterialQualityLevel ' + $materialQualityLevel
$legacyCommands=@()
if ($ScalabilityTier -eq 0) {
    # 只作用于本次独立采样；检查不依赖 Nanite/VSM/Lumen 的回退成本，不覆盖项目或玩家设置。
    $legacyCommands=@('r.Nanite 0','r.Shadow.Virtual.Enable 0','r.Lumen.DiffuseIndirect.Allow 0','r.Lumen.Reflections.Allow 0','r.AntiAliasingMethod 2')
    $quality+=','+($legacyCommands -join ',')
}
$viewCommands = for ($cameraIndex=0; $cameraIndex -lt $cameras.Count; $cameraIndex++) { [string](600+$cameraIndex*1200) + ':ViewActor ' + $cameras[$cameraIndex].name }
$tierGroups=@('ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage')
$tierCommands=for ($i=0;$i -lt $tierGroups.Count;$i++) { [string](330+$i)+':sg.'+$tierGroups[$i]+'Quality '+$ScalabilityTier }
$views = '300:r.SetRes 1920x1080w,320:r.ScreenPercentage 100,' + ($tierCommands -join ',') + ',' + ($viewCommands -join ',')
if ($legacyCommands.Count) {
    $legacyFrames=for ($i=0;$i -lt $legacyCommands.Count;$i++) { [string](340+$i)+':'+$legacyCommands[$i] }
    $views+=','+($legacyFrames -join ',')
}
# 截图放在全部稳定段之后，避免截图读回成本污染性能统计；不改玩家截图设置。
if ($CaptureReview) {
    for ($i=0;$i -lt $ReviewLabels.Count;$i++) {
        $reviewCamera=$cameras | Where-Object { $_.label -eq $ReviewLabels[$i] } | Select-Object -First 1
        if (-not $reviewCamera) { throw ('缺少评审相机 '+$ReviewLabels[$i]) }
        $reviewStart=$captureFrames+$i*240
        $shotName='HomeMap_'+$qualityName+'_'+$ReviewLabels[$i]+'_'+$stamp
        $views+=','+[string]$reviewStart+':ViewActor '+$reviewCamera.name+','+[string]($reviewStart+180)+':Shot filename='+$shotName
    }
    $captureFrames+=$ReviewLabels.Count*240
    $quality=$quality -replace 'csvprofile frames=\d+',('csvprofile frames='+$captureFrames)
}
# 独立游戏进程读取正式地图。使用清单中的环境评审相机，不更改玩家 Camera 实现。
# 不使用 boot capture；它会在解析项目路径前选中引擎用户目录。进入项目后启动 CSV。
$arguments = @('"' + $projectFile + '"', '/Game/Environment/HomeMap/Maps/HomeMap_Courtyard', '-game', '-PIEVIACONSOLE', '-Multiprocess', 'GameUserSettingsINI=HomeMapPavilionProfile', '-forcepassthrough', '-RenderOffscreen', '-unattended', '-nosound', '-NoSplash', '-ResX=1920', '-ResY=1080', '-windowed', '-novsync', '-csvGpuStats', '-ExitAfterCsvProfiling', '-ExecCmds="' + $quality + '"', '-csvExecCmds="' + $views + '"', '-abslog="' + $logPath + '"')
$process = Start-Process -FilePath $editorProcess.ExecutablePath -ArgumentList $arguments -WindowStyle Hidden -PassThru
$aaMode=if ($ScalabilityTier -eq 0) { 'TAA; Nanite/VSM/Lumen disabled' } else { 'project-default TSR' }
$report = @{ process_id = $process.Id; log = $logPath.Substring($projectRoot.Length + 1); started = (Get-Date).ToString('o'); views = $views; resolution = @(1920,1080); frames = $captureFrames; mode = ('Independent game process, RenderOffscreen, '+$qualityName+', '+$aaMode) }
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $projectRoot 'Saved/HomeMapCheckpoints/pavilion_profile_process.json') -Encoding UTF8
$report | ConvertTo-Json -Depth 5
