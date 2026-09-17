# 编译指南

本指南用于 NewWorldOrder 工程的本地编译与引擎版本定位。

## 引擎版本

- 以 NewWorldOrder.uproject 的 EngineAssociation 为准
- EngineAssociation 变更后，需要同步替换引擎路径

读取方式（PowerShell 示例）：
```
$uproject = Get-Content .\NewWorldOrder.uproject | ConvertFrom-Json
$engineAssoc = $uproject.EngineAssociation
```

## Windows 编译

优先顺序（不写死盘符）：
1. UE_ENGINE_DIR 环境变量（手动指定）
2. 注册表 HKCU/HKLM 的 Unreal Engine\Builds
3. 常见安装目录扫描（Program Files/Epic Games/UE_版本）

完整示例（PowerShell）：
```
$uproject = Get-Content .\NewWorldOrder.uproject | ConvertFrom-Json
$engineAssoc = $uproject.EngineAssociation
$engineDir = $env:UE_ENGINE_DIR

if (-not $engineDir)
{
    $buildsHkcu = Get-ItemProperty "HKCU:\Software\Epic Games\Unreal Engine\Builds" -ErrorAction SilentlyContinue
    $buildsHklm = Get-ItemProperty "HKLM:\Software\Epic Games\Unreal Engine\Builds" -ErrorAction SilentlyContinue
    $engineDir = @($buildsHkcu.$engineAssoc, $buildsHklm.$engineAssoc) | Where-Object { $_ } | Select-Object -First 1
}

if (-not $engineDir)
{
    $roots = @(
        "$env:ProgramFiles\Epic Games",
        "$env:ProgramFiles(x86)\Epic Games",
        "C:\Program Files\Epic Games",
        "D:\Program Files\Epic Games",
        "E:\Program Files\Epic Games",
        "F:\Program Files\Epic Games",
        "G:\Program Files\Epic Games"
    ) | Select-Object -Unique

    foreach ($root in $roots)
    {
        if (-not $root) { continue }
        $candidate = Join-Path $root ("UE_{0}" -f $engineAssoc)
        if (Test-Path $candidate)
        {
            $engineDir = $candidate
            break
        }
    }
}

if (-not $engineDir)
{
    throw "EngineDirNotFound. Set UE_ENGINE_DIR or register EngineAssociation in Unreal Engine\\Builds."
}

& "$engineDir\Engine\Build\BatchFiles\Build.bat" `
  NewWorldOrderEditor Win64 Development `
  -Project="$(Resolve-Path -LiteralPath '.\NewWorldOrder.uproject')" `
  -WaitMutex
```

推荐脚本（无需写死盘符）：
```
.\Scripts\Build_Windows.ps1
```

可选参数：
```
.\Scripts\Build_Windows.ps1 -Configuration Development -Platform Win64 -Target NewWorldOrderEditor
```

需要由脚本完成“关闭目标项目、冷编译、重新启动并等待 MCP”时：
```
.\Scripts\Build_Windows.ps1 -RestartEditor -WaitForReady
```

- `-RestartEditor` 只匹配 `UnrealEditor.exe` 命令行中的完整 `NewWorldOrder.uproject` 绝对路径；不会按进程名批量关闭 Lyra 或其他 Unreal 项目。
- 项目路径含空格，重新启动时脚本把 `.uproject` 保留为带引号的单一参数，避免误开 Project Browser。
- 不带 `-RestartEditor` 且检测到 NewWorldOrder 编辑器仍在运行时，脚本会明确失败；这样不会把带反射变更的代码误编成 Hot Reload 临时 DLL。
- `-Map /Game/Maps/MapName` 可指定重启后地图；`-WaitForReady` 等待 `Saved/VibeUE/Signals` 的目标 PID readiness signal。
- 该脚本没有复制 `Plugins/VibeUE/BuildAndLaunchGame.ps1` 的全局 Unreal 进程清理与日志清理步骤。后者属于上游插件通用工具，会关闭所有 Unreal 编辑器，不适合本项目与 Lyra 并行工作的场景。

常见问题：
- Visual Studio 工具链提示非首选版本属于警告，可继续编译
- 若提示插件依赖警告但不报错，可先忽略

## Windows 打包

本项目 UE 5.8 在这台工作站上存在一个已验证的 Zen IPv6 回环兼容问题：Cook 生成的 `Saved/Cooked/Windows/ue.projectstore` 使用 `[::1]:8558`，UnrealPak 能访问 Zen，但 AutomationTool 随附的 .NET 10 健康检查会返回失败。日志因此误报 `ZenServer is not running`，即使 Zen 正在监听且 `/health/ready` 返回 200。

用户从编辑器 `Platforms -> Package Project` 打包，因此项目在 `Config/DefaultGame.ini` 的 `UProjectPackagingSettings` 上设置 `bUseZenStore=False`。这是 Project Settings -> Packaging 中 `Use Zen Server as cooked output store` 的官方配置：

- Cook 中间产物改为写入本地文件，不再生成需要 UAT 读取的 Zen oplog。
- `UsePakFile` 和 `bUseIoStore` 仍保持引擎默认开启，最终包继续生成 `.pak/.ucas/.utoc`。
- 代价是 Cook 可能比 Zen 增量流程稍慢，但编辑器打包不再依赖当前故障的 `[::1]` 链路。
- 不修改 `Saved` 默认值、UE 引擎源码或插件源码；`Saved` 仍由每次 Cook 自动生成。

修改配置或插件清单后需要重启编辑器，再从菜单选择 Development 或 Shipping。不要使用旧 Cook 目录判断新配置是否生效；UAT 命令行中不应再出现 `-zenstore`。

当前动画资产 `/Game/UI/Mutable/Animation/ABP_WardrobePreview_Female` 序列化了 `MovieSceneAnimMixer` 的 Anim Blueprint Extension，因此运行时插件 `MovieSceneAnimMixer` 必须启用。编辑器工具插件 `SequencerAnimMixerToolset`、`VibeUE`、MCP 和其他 AI Toolset 可以在打包配置中禁用；开发期若需通过 MCP 编辑蓝图，则按实际需要重新启用对应插件并重启编辑器。

## macOS 编译

常见引擎路径：
- /Users/Shared/Epic Games/UE_5.8

示例：
```
UE_ENGINE_DIR="/Users/Shared/Epic Games/UE_5.8"
"$UE_ENGINE_DIR/Engine/Build/BatchFiles/Mac/Build.sh" \
  NewWorldOrderEditor Mac Development \
  -Project="/path/to/NewWorldOrder.uproject" \
  -WaitMutex
```

## 输出位置

编译产物与中间文件位于：
- Intermediate/
- Binaries/

若需要清理，可删除上述目录后重新编译。
