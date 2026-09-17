# IconGeneration 图标生成

最后更新：2026-07-07

## 目标

姿势库条目的图标来自动画资产预览图，不需要手工截图。

当前脚本：

- `Scripts/Generate_PoseLibraryIcons.ps1`

默认处理：

- 动画来源：`/Game/Assets/Animations/Girl`
- 临时 PNG 输出：`Saved/PoseIconSource`
- Texture2D 导入目录：`/Game/Assets/Animations/PoseIcons`
- DataAsset：`/Game/Blueprints/Animations/DA_PoseLibrary`

## 使用方法

先打开 UE 编辑器并确认项目 MCP 服务已启动，然后在项目根目录运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\Generate_PoseLibraryIcons.ps1
```

只重新抓取 PNG，不导入和回填 DataAsset：

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\Generate_PoseLibraryIcons.ps1 -SkipImport
```

已有 PNG，只重新导入并回填 DataAsset：

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\Generate_PoseLibraryIcons.ps1 -SkipCapture
```

## 实现方法

脚本分三步：

1. 通过 MCP 初始化会话，拿到 `Mcp-Session-Id`。
2. 对 `/Game/Assets/Animations/Girl` 下每个 `.uasset` 调用 `EditorToolset.EditorAppToolset.CaptureAssetImage`，把返回的 base64 PNG 写到 `Saved/PoseIconSource`。
3. 通过 `execute_python_code` 调用 Unreal Python，把 PNG 批量导入为 Texture2D，再按动画名匹配 `DA_PoseLibrary.PoseEntries[*].Animation`，写回 `Icon` 字段。

导入后的命名规则：

- `Female_Action_Pose.png` 导入为 `/Game/Assets/Animations/PoseIcons/T_PoseIcon_Female_Action_Pose`
- `DA_PoseLibrary` 条目里如果动画是 `Female_Action_Pose`，脚本会把对应 Texture2D 填到该条目的 `Icon`

## 注意事项

- 该方案使用编辑器自带资产预览截图，不进入 PIE。
- 每次 MCP 请求之间有短暂停顿，避免连续请求把编辑器压崩。
- 之前尝试过 SceneCapture2D 加 RenderTarget 的批处理方案，当前项目编辑器环境下发生过崩溃；姿势库图标统一使用 `CaptureAssetImage`。
- 新增姿势动画后，先把动画加入 `/Game/Blueprints/Animations/DA_PoseLibrary.PoseEntries`，再运行脚本生成和回填图标。
- 如果图标生成后效果不理想，可以只替换 `Saved/PoseIconSource` 里对应 PNG，然后用 `-SkipCapture` 重新导入和回填。

