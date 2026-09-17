# MCP (ModelContextProtocol) 踩坑记录

日期：2026-08-15（最后更新）
状态：[可用]

当前权威版本：UE 5.8.1，工作站实测 `5.8.1-56057345+++UE5+Release-5.8`。`NewWorldOrder.uproject` 的 `EngineAssociation` 为 `5.8`，该字段不包含补丁号，不能据此误判为 5.8.0。

---

## 快速启动（新会话 AI 必读）

AGENTS.md 第 6 步路由到此文档。

### 初始化（1 轮就绪）

直接 `initialize` 获取 Session ID 后即可调用任何工具：

```bash
# 1. 写 JSON 到文件
# 2. 发送请求，从响应头取 Mcp-Session-Id
curl.exe -s -i -X POST http://localhost:8000/mcp \
  -H "Content-Type: application/json" \
  --data-binary @_mcp_req.json
```

请求体：
```json
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"reasonix","version":"1.0"}}}
```

排查提示：若 `/mcp` 返回 404 `route_handler_not_found`，说明 MCP 路由不在该端口（可能被 `ModelContextProtocol.StartServer <port>` 切走）；执行 `ModelContextProtocol.StartServer 8000` 切回即可。TCP 通 + 404 不等于 MCP 挂起。

### 调用工具

正式版统一格式：`call_tool` + `toolset_name` + `tool_name`：

```json
{
  "method": "tools/call",
  "params": {
    "name": "call_tool",
    "arguments": {
      "toolset_name": "UMGToolSet.UMGToolSet",
      "tool_name": "AddWidget",
      "arguments": { /* 实际参数 */ }
    }
  }
}
```

### curl 模板

```bash
curl.exe -s -X POST http://localhost:8000/mcp \
  -H "Content-Type: application/json" \
  -H "Mcp-Session-Id: <SESSION_ID>" \
  --data-binary @_mcp_req.json
```

不要用 PowerShell 的 `curl`（那是 Invoke-WebRequest 别名），必须用 `curl.exe`。

PowerShell 中不要把 JSON 字符串直接管给 `curl.exe --data-binary @-`。PowerShell 管道可能按 UTF-16 传输文本，UE MCP 会返回 `Invalid JSON body`。可靠做法是把请求写入无 BOM UTF-8 临时文件，再用 `--data-binary "@_mcp_req.json"` 发送。

环境差异：以上是 Windows 用法。WSL/bash 或 macOS 下把 `curl.exe` 换成 `curl`，用 `printf`/`cat` 写无 BOM UTF-8 请求文件；路径一律用相对项目根目录的写法，不要照抄 PowerShell 示例或硬编码盘符。

示例：

```powershell
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$json = $payload | ConvertTo-Json -Depth 50 -Compress
[System.IO.File]::WriteAllText("_mcp_req.json", $json, $utf8NoBom)
curl.exe -s -X POST http://localhost:8000/mcp `
  -H "Content-Type: application/json" `
  -H "Mcp-Session-Id: <SESSION_ID>" `
  --data-binary "@_mcp_req.json"
```

UE 5.8.1 已修复 `tools/call` 在有进度和无进度路径下的响应 framing。调用端仍需根据响应 `Content-Type` 同时兼容 JSON 与 SSE：若为 SSE，取 `data:` 行后再 `ConvertFrom-Json`；若为 JSON，直接解析响应体。不要再把 5.8.0 的 framing 异常当成工具业务失败，也不要无条件把所有响应按 SSE 截取。

### 核心约束

- 仅用 curl，禁止 Python urllib/requests（会触发 UE HTTP 服务器断言崩溃）
- 每次只发一个请求并等待该请求返回；不再固定等待 1-2 秒。2026-07-26 正式版 MCP 实测单个只读/配置脚本常在 20-700 ms 内完成，状态轮询可按 0.5-1 秒节奏进行。若编辑器出现模态框或工具仍在执行，必须先处理/等待，不能并发堆叠请求。
- UE 5.8.1 在工具脚本执行期间禁用事务，脚本产生的多项修改不会再被打包成一个可依赖的 Undo 事务。写入脚本必须幂等，逐项打印 `CREATED/ADDED/MODIFIED/DELETED` 路径，并按逻辑阶段编译、保存、复读和建立 Git 检查点。
- UE 5.8.1 已修复向没有 `SimpleConstructionScript` 的 Blueprint 添加组件时的致命崩溃。不要为该 5.8.0 缺陷采用删除重建 Blueprint、清空父类或人为补 SCS 的旧绕行方案；仍须在写入后编译并复读组件结果。
- 每 5 个 UMG AddWidget 后必须 CompileWidgetBlueprint

---

## Toolset 命名速查

| 功能 | 正式版 Toolset Name |
|------|-------------------|
| UMG 控件树 | `UMGToolSet.UMGToolSet` |
| Blueprint | `editor_toolset.toolsets.blueprint.BlueprintTools` |
| Object 属性 | `editor_toolset.toolsets.object.ObjectTools` |
| Asset 操作 | `editor_toolset.toolsets.asset.AssetTools` |
| Python 脚本 | `editor_toolset.toolsets.programmatic.ProgrammaticToolset` |
| Data Asset | `editor_toolset.toolsets.data_asset.DataAssetTools` |
| Live Coding | `LiveCodingToolset.LiveCodingToolset` |

## VibeUE 5.0 入口确认（2026-06-29）

用户执行 `VibeUE.GenerateAgentConfig codex` 和 `ModelContextProtocol.GenerateClientConfig codex` 后，当前项目的 `.codex/config.toml` 指向 `http://127.0.0.1:8000/mcp`。VibeUE 不启动独立服务器，而是把工具注册进 UE 5.8.1 原生 MCP 端点。

当前 `tools/list` 已验证可用工具：

| 工具名 | 用途 |
|------|------|
| `execute_python_code` | 在编辑器内执行 Python，必须以 `import unreal` 开头 |
| `discover_python_class` | 查询 `unreal.*` 类和 VibeUE Service 的真实方法签名 |
| `discover_python_function` | 查询 Python 函数签名 |
| `discover_python_module` | 查询模块 |
| `list_python_subsystems` | 查询编辑器 Python 子系统 |
| `list_toolsets` | 查询 Toolset，但实测可能耗时较长 |
| `describe_toolset` | 查询 Toolset schema，信息量很大，避免常规使用 |

本机 `Engine/Build/Build.version` 与项目启动日志已交叉验证为 `5.8.1-56057345+++UE5+Release-5.8`。后续 UMG、蓝图、截图、PIE 操作优先用 `execute_python_code` 批处理，截图这类需要图片返回的操作再走原生 `call_tool`。

---

## UMG 工具速查

| 工具名（不带前缀） | 用途 |
|---------|------|
| `AddWidget` | 添加控件。参数：`widgetBlueprint`, `widgetClass`, `widgetDisplayName`, `parentWidget`(可选), `childIndex`(可选) |
| `RemoveWidget` | 删除控件及子控件 |
| `MoveWidget` | 移动控件到新父控件 |
| `WrapWidgets` | 将一个或多个控件包裹进 Panel（等效编辑器右键 Wrap With） |
| `CompileWidgetBlueprint` | 编译。返回 true 才算成功 |
| `GetWidgets` | 读取完整控件树 |
| `CreateWidgetBlueprint` | 创建新 Widget Blueprint。参数：`folderPath`, `assetName`, `parentClass` |
| `ReparentWidgetBlueprint` | 改变 Widget Blueprint 的 C++ 父类 |
| `SetNamedSlotContent` | 设置命名槽位内容 |
| `ListWidgetClasses` | 列出可用控件类 |
| `BindToEventProperty` | 绑定控件事件到 Blueprint 图 |

ObjectTools 的 `list_properties` / `get_properties` / `set_properties` 用于读写控件样式。对每个 Widget 必须先 `list_properties` 获取准确属性名才能 `set_properties`。

---

## 已知问题

### EditorModeManager 断言崩溃

打开 Widget Blueprint 编辑器时偶发。规避：用 MCP 修改 Widget Blueprint 后优先通过 MCP 的 GetWidgets 查看结构，避免双击打开编辑器。

### HTTP 服务器连接状态断言（Python 触发）

Python urllib/requests 发送 MCP 请求会触发 UE HTTP 服务器断言。强制只用 curl。

### tools/list 可能卡住双编辑器

同时打开两个带 MCP 的 UE 编辑器时，避免频繁调用 `tools/list`。优先直接调用已知工具名。

### macOS UE 5.8.1 StartPIE 仍需保守处理

该问题最初在 2026-07-03 的 UE 5.8.0 macOS 环境复现；5.8.1 发布说明没有声明修复 StartPIE 挂起，因此在完成 5.8.1 macOS 复测前继续按以下保守规则处理：

处理规则：

- 发现 MCP 无响应时，先用 `pgrep -fl UnrealEditor` 和 `lsof -nP -iTCP:8000 -sTCP:LISTEN` 检查编辑器进程与 MCP 端口。
- 如果 UnrealEditor 进程消失，停止任务并让用户重启编辑器。
- 如果 UnrealEditor 进程仍在但 PIE 请求长期不返回，不要继续反复启动 PIE。
- 可以继续做 VibeUE/MCP 资产级验收、蓝图编译状态检查、代码编译和文档更新。
- 不允许把资产级验收写成 PIE 验收；Host/Client 双端可见性必须在稳定 PIE 或独立运行环境中重新验证。

### VibeUE capture_preview 不是 Designer 截图

`unreal.WidgetService.capture_preview(widget_path, width, height)` 来自 VibeUE 的 `WidgetService`，不是 UE 原生 UMGToolSet。它会离屏实例化 Widget，并执行 `NativePreConstruct` / `NativeConstruct` 等运行时逻辑，因此结果可能与 Widget Designer 面板不同。

典型差异：

- Designer 中显示的是编辑器画布、选中框、缩放和未运行状态；`capture_preview` 只渲染运行时 Widget。
- 根节点透明时，离屏预览背景可能显示为黑色。
- C++ 父类在 `NativePreConstruct` 中写 UI 文本时，`capture_preview` 会看到运行时文本，而不是 Designer 手动填的示例文本。
- 对复杂 UMG 层级，`capture_preview` 可能出现布局预通道与 Designer 不一致的问题，例如文本被压到左上角，但 Designer 中层级和样式正常。

结论：`capture_preview` 可用于快速发现明显结构错误，但不能作为 UMG 最终视觉验收依据。最终以 Widget Designer、PIE 运行态截图和玩家实际交互为准。

### UE 5.8.1 CompileWidgetBlueprint 参数

2026-06-30 实测：`UMGToolSet.UMGToolSet.CompileWidgetBlueprint` 的 schema 要求 `widgetBlueprint.refPath`，旧的 `WidgetBlueprint="/Game/..."` 字符串参数会被工具层当成空参数并报错。

如果 `call_tool` 包装层没有正确传入对象参数，可改用 `execute_python_code` 批处理：

```python
import unreal
wbp = unreal.EditorAssetLibrary.load_asset("/Game/UI/Mutable/W_Cloth.W_Cloth")
unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
unreal.EditorAssetLibrary.save_asset("/Game/UI/Mutable/W_Cloth", only_if_is_dirty=False)
```

说明：这是 5.8.1 下 `call_tool` 对象参数转发不稳定时的安全兜底，不是替代 UMGToolSet 的长期方案。仍需通过 `EditorAssetLibrary.save_asset` 明确保存资产。

### UE 5.8.1 WidgetBlueprint 生成类读取差异

2026-06-30 实测：直接读 `WidgetBlueprint.generated_class` 或 `get_editor_property("generated_class")` 可能失败。需要读取 Widget CDO 时，优先使用生成类对象路径：

```python
import unreal
gen = unreal.load_object(None, "/Game/UI/Mutable/W_Cloth.W_Cloth_C")
cdo = unreal.get_default_object(gen)
print(cdo.get_editor_property("input_mapping"))
```

同轮还确认 `UInputMappingContext` 的旧 `mappings` 属性可能为空，新数据在 `default_key_mappings.mappings` 结构内。

### Live Coding 未启用

`LiveCodingToolset.LiveCodingToolset.CompileLiveCoding` 需要在编辑器偏好中启用 Live Coding。

### 同引擎另一个编辑器实例会让项目构建变成 Hot Reload

2026-07-26 起持续适用：即使 NewWorldOrder 编辑器已经关闭，只要同一 UE 5.8.1 可执行文件仍运行着 Lyra 编辑器，UBT 也可能把命令行构建判定为 Hot Reload，生成 `UnrealEditor-NewWorldOrder-000N.dll`。随后重启项目时只加载基础 DLL，新加的 UCLASS 会在 Python 反射中表现为不存在。

需要保留 Lyra 11000 实例并做真正冷编译时，构建参数必须包含：

```powershell
-NoHotReloadFromIDE
```

验收标准：

- 构建输出生成 `UnrealEditor-NewWorldOrder.dll`，不是 `-000N.dll`。
- 输出显示删除旧 hot-reload 文件。
- 项目重启后重新 initialize 8000 会话，再检查新增类反射；不能继续使用关闭前的 MCP Session Id。

### 删除 UMG 变量控件后首次编译出现旧 GUID Ensure

UE 5.8.1 中用 Widget Service 删除勾选为变量的 UMG 控件后，首次编译可能提示该变量已删除但 Blueprint 仍持有旧 GUID。Widget Blueprint Compiler 会在该次编译中移除残留引用；应立即再次编译并确认返回成功、状态为 UpToDate，不能把第一次 Ensure 当作最终资产状态。若第二次仍出现相同 Ensure，停止批量修改并检查图表、动画或绑定是否仍引用该控件。

### WidgetService.bind_event 返回成功但事件可能不持久化

2026-07-23 在 `W_SessionBrowserScreen.BackButton` 上复现：`WidgetService.bind_event()` 返回成功，当前编辑器进程内也能看到相关结果，但重启编辑器后 EventGraph 没有运行时有效的按钮事件。仅凭该调用返回值、蓝图 UpToDate 或 hierarchy validation 不能判定按钮可用。

固定 Widget 按钮应使用 Designer 绿色加号等价的 component-bound event：

```python
event_id = unreal.BlueprintService.create_component_bound_event(
    "/Game/UI/Menu/Experiences/W_SessionBrowserScreen",
    "EventGraph",
    "BackButton",
    "OnButtonBaseClicked",
    100,
    100)
```

然后显式创建业务函数调用并连接 `then -> execute`，编译、保存、重启编辑器，再次读取节点确认：

- 节点类型是 `K2Node_ComponentBoundEvent`。
- `On Clicked (BackButton)` 的 `then` 确实连接到业务节点。
- PIE 中广播或真实点击该按钮能触发页面退栈。

手工用 `create_node_by_key` 拼一个外观相似的事件节点也不等价；缺少编译器需要的组件委托绑定参数时，节点可以编译但运行时不会触发。

### 未连接的 Widget Event Tick 会被裁剪

2026-08-12 在 `Interactive_Progress_Bar` 上确认：仅在 C++ Widget 基类重写 `NativeTick`，不能保证 Widget Blueprint 会生成并启用 Tick。若蓝图里的 `Event Tick` 没有执行连接，编译器可能裁剪该路径，表现为 C++ 刷新代码正确、Widget 实例存在，但进度条始终不更新。

对于确实需要逐帧刷新的轻量 HUD 片段，保留语义明确的 Widget Blueprint 调用链：

1. 在 C++ 基类暴露一个 `BlueprintCallable` 刷新函数，只负责读取本地状态并更新可选绑定控件。
2. 在 Widget Blueprint 中把 `Event Tick` 连接到该语义函数。
3. 编译两次、保存，并复读节点与连接；PIE 中同时读取业务状态和真实控件的 Visibility/Percent，不能只看 Controller 字段。

Lyra 的 `/ShooterCore/UserInterface/HUD/W_WeaponReticleHost` 也使用蓝图 `Event Tick` 调用语义刷新函数，可作为项目 HUD Host 的对照模式。若状态本身有离散消息或 FieldNotify，仍应优先事件驱动；不要把所有 UI 都改成 Tick。

### Git LFS 过滤器触发 sh 启动失败（DSH 沙箱）

2026-08 实测：仓库 `.gitattributes` 把 `*.uasset` 等声明为 `filter=lfs`，`git status` / `git diff` 会启动 `git-lfs filter-process`（经 `sh.exe`）。在禁用命名管道的执行环境里 sh 启动即报 `couldn't create signal pipe, Win32 error 5`，导致 status/diff 整体失败，容易误判为仓库损坏。

只读检查可用旁路参数（不改变任何文件）：

```powershell
git -c filter.lfs.process= -c filter.lfs.required=false -c filter.lfs.smudge=cat -c filter.lfs.clean=cat status --short
```

注意：`smudge=cat / clean=cat` 会让 LFS 资产在对比中表现为“已修改”假阳性。判断 uasset 是否真的被改，用磁盘 SHA256 对比 HEAD 中 LFS 指针的 `oid sha256:` 值。提交时只 `git add` 具体的文本/代码路径，不要用过滤器旁路状态暂存 LFS 资产。

### 沙箱内跑 UBT 会触发 dotnet.exe 崩溃

2026-08-14 实测：在 DSH 沙箱（禁用命名管道）里执行 `Scripts/Build_Windows.ps1`，UnrealBuildTool 的 dotnet 进程抛 `0xe0434352`（.NET 运行时异常，进程退出码 `-532462766`），构建中途静默失败且无完整日志。用户在本机普通终端直接编译成功。

结论：UBT 编译/链接需要子进程管道通信，在沙箱内不可靠。应在沙箱外（用户终端）编译，或由用户代为编译后，AI 用 `Binaries/Win64/UnrealEditor-NewWorldOrder.dll` 的 LastWriteTime 验证是否产出新 DLL。

### Live Coding：小改可用，崩溃后必须转冷编译

2026-08-14 两轮实测推翻早前"Live Coding 可在沙箱内直接调用"的结论：

- 首次：加 UPROPERTY + 新 include 后调 `LiveCodingToolset.CompileLiveCoding` 返回 Success，但重启/加载时崩溃 `dllmain_crt_process_attach EXCEPTION_ACCESS_VIOLATION`（CrashContext 指向 `UnrealEditor-NewWorldOrder.patch_0!dllmain_crt_process_attach`）。
- 再次：仅新增**纯函数**（`RotatePreviewByDelta`，无 UPROPERTY、无新 include）仍复现同一崩溃（patch_0 21:31:51 生成，21:31:57 崩溃）。

结论：本环境 Live Coding 有两次 patch DLL 崩溃前科，但快速小改仍可先试；一旦出现 patch_* 崩溃或编辑器冻结，立即清理 patch 残留并转冷编译，不再重试。**冷编译在 danger-full-access 沙箱内可直接跑通**（2026-08-14 实测 `Scripts/Build_Windows.ps1` 47.5s 完成，UBT dotnet 正常；workspace-write 下才抛 0xe0434352）。流程：杀 `LiveCodingConsole`/`CrashReportClientEditor`（会锁 patch 文件）→ 删 `Binaries/Win64/*.patch_*` → 后台跑 Build_Windows.ps1 → 重启编辑器 → 重新 initialize 拿新 Session Id → Python 反射验证新函数。

### MCP“只读”蓝图工具可能重存资产

2026-08-14 实测：`BlueprintTools.read_graph_dsl` / `list_events` 与 `UMGToolSet.GetWidgets` 读取 `W_Cloth` / `W_Cloth_Item` 后，两个 uasset 的 LastWriteTime 变为读取时刻，磁盘字节与 HEAD 的 LFS 内容不再一致（Widget Blueprint 重编译并落盘）。语义内容未确认变化，但工作区会因此出现非预期 M。

读资产前先记录目标 uasset 的 LastWriteTime；读完后若出现 M，用“磁盘 SHA256 vs LFS oid”确认，并对比时间点判断是重存噪音还是真实改动。不要把这些重存产物当作玩家的编辑结果直接提交。

### UE 5.8 AssetData 类型判断不能比较 TopLevelAssetPath 字符串

2026-08-24 实测：`str(AssetData.asset_class_path)` 返回包含内存地址和结构字段的调试文本，使用 `endswith('.ObjectRedirector')` 或 `endswith('.ParticleSystem')` 会把全部资产误分类。固定读取 `str(AssetData.asset_class_path.asset_name)`，例如 `ObjectRedirector`、`ParticleSystem`；在任何 Fix Up、删除或迁移判断前先输出各路径的 class count 和样本验证口径。

---

## MCP 无响应排查（按顺序，均已实测）

- 先确认 8000 属于目标项目的编辑器实例（Lyra 11000 等其他项目实例可并存）；localhost 请求被代理劫持时才加 `--noproxy "*"`。
- initialize / ping 是同步路径（ModelContextProtocolServer.cpp:558），能正常返回说明 HTTP 与 MCP 层都活着。
- ping 通但 execute_python_code 挂：工具要回游戏线程执行（AsyncTask(GameThread)，同文件:958），先查编辑器进程 Responding/CPU 是否冻结或繁忙；Live Coding patch DLL 崩溃会冻结编辑器，出现后转冷编译。
- TCP 通但 `/mcp` 返回 404 `route_handler_not_found`：MCP 路由不在该端口，执行 `ModelContextProtocol.StartServer 8000` 切回。
- 以上都排除后再怀疑引擎；不要先动重启编辑器、重装引擎这类大动作。

---

## 相关日志路径

```
Saved/Crashes/UECC-Windows-*/CrashContext.runtime-xml
Saved/Crashes/UECC-Windows-*/NewWorldOrder.log
```

## 相关代码（UE 引擎源码，只读参考）

- `Engine/Source/Editor/UnrealEd/Private/EditorModeManager.cpp` Line 658
- `Engine/Source/Runtime/Online/HTTPServer/Private/HttpConnection.cpp` Line 184

## 相关工具

- `Docs/DevelopmentNotes/generate_toolsets.py`：扫描引擎源码提取 MCP 工具定义，生成 JSON 速查表
- `Docs/DevelopmentNotes/MCP_AddWidget_Nesting_Bug_验证报告.md`：预览版嵌套 bug 的 14 次测试记录（正式版已修复）
