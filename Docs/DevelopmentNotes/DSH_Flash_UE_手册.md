# DSH Flash 模型 UE 实战手册

> 面向 DeepSeek v4 Flash（DSH 沙箱环境）在 NewWorldOrder 项目中的 UE 开发协作手册。
> 内容为 2026-08-14 会话中实测验证的经验，供后续 AI 会话快速恢复上下文。
> 每条按「问题 → 已验证结论 → 证据/来源 → 时效性标注」组织。
> 时效性标注：**稳定** = 引擎/工具行为不易变；**版本相关** = 随 UE/VibeUE 版本可能变；**推断** = 从源码或行为推断；**可能过时** = 环境变化后需复核。

---

## 1. 沙箱内编译：冷编译在 danger-full-access 下可用；Live Coding 在本环境不可靠（勿用）

**问题**：DSH 沙箱里改 C++ 后如何编译？

**已验证结论（2026-08-14 两轮实测修订）**：
- **Live Coding 在本项目/本环境不可靠，已弃用**：workspace-write 模式下 LiveCodingToolset.CompileLiveCoding 虽能返回 Success，但生成的 patch DLL（UnrealEditor-NewWorldOrder.patch_0.*）在加载时崩溃（dllmain_crt_process_attach EXCEPTION_ACCESS_VIOLATION）。不止 UPROPERTY/结构改动会触发，**新增纯函数（无 UPROPERTY、无新 include）同样崩溃**。崩溃后编辑器退出，重启前必须清理 Binaries/Win64/*.patch_* 残留。
- **冷编译（Scripts/Build_Windows.ps1）在 danger-full-access 下可直接跑通**：本次实测 47.5s 完成（UBT dotnet 正常，无 0xe0434352）。workspace-write 下 UBT 会抛 0xe0434352（禁命名管道所致），需用户在 full-access 会话或本机终端编译。
- **改 C++ 一律走冷编译**：编辑器关闭状态（或容忍重启）→ 清理 patch 残留 → 后台跑 Build_Windows.ps1 → 重新启动编辑器 → 重新 initialize MCP 拿新 Session Id → 用 Python 反射验证新函数已注册。
- 清理 patch 残留注意：崩溃后 CrashReportClientEditor 进程可能锁住 patch 文件，Remove-Item 报 Access denied；先 Stop-Process -Name CrashReportClientEditor -Force 再删。LiveCodingConsole 进程也要杀。

**证据**：2026-08-14 同日两次 Live Coding 崩溃（patch_0!dllmain_crt_process_attach，含纯函数场景）；同日在 full-access 下冷编译 RotatePreviewByDelta 成功（Result: Succeeded，47.5s）。记录于 MCP_踩坑记录.md。

**时效性**：**版本相关**（UE 5.8.1 / DSH 沙箱行为）。若某天 Live Coding 恢复正常可重新评估，但当前一律冷编译。

---

## 2. git push 需要特殊配置（沙箱限制）

**问题**：沙箱内 git push 为什么失败？如何推送？

**已验证结论**：
- 根因：Git for Windows 的 msys sh/ssh spawn 子进程需要创建命名管道，被沙箱（workspace-write）拦截，报 sh.exe: fatal error - couldn't create signal pipe。
- **解法**：用 Windows 原生 OpenSSH + 正斜杠路径 + BatchMode：
```powershell
$env:GIT_SSH_COMMAND = "C:/Windows/System32/OpenSSH/ssh.exe -o BatchMode=yes -o ConnectTimeout=30"
git -C <repo> push origin main
```
- 若权限仍是 ask 模式，push 需要升级到 danger-full-access（DSH 的授权只有 ask/never 两种，无 per-command 白名单）。
- 本地 git 操作（status/add/commit）可加 LFS 旁路避免 msys sh 崩溃：
```powershell
git -c filter.lfs.process= -c filter.lfs.required=false -c filter.lfs.smudge=cat -c filter.lfs.clean=cat status --short
```

**证据**：2026-08-14 多次实测；DSH 源码 packages/bundle/base/cordis.patch.yml。

**时效性**：**版本相关**（Git for Windows / DSH 沙箱行为）。用户切 danger-full-access 后无需再升级。

---

## 3. 蓝图图操作：write_graph_dsl / build_graph 的坑

**问题**：MCP 修改蓝图 EventGraph 的正确姿势？

**已验证结论**：
- BlueprintTools.write_graph_dsl 可用，但**会覆盖整个图**；对已有复杂图重写可能因类型推断差异失败（如 Could not connect pin bLocked to self）。**不要在已有复杂图上用 write 覆盖**，用 build_graph 增量追加。
- BlueprintService.build_graph 任一条连接失败即整体返回 None（无逐项错误），需单独创建节点 + connect_nodes 排查。
- **节点 ID 必须完整 GUID**（32 位 hex），截断 8 位会导致连接返回 False。
- 蓝图自定义事件作为函数调用时，function_call 的 class 需完整资产路径（如 /Game/UI/Mutable/W_ClothItemTooltip.W_ClothItemTooltip_C），短类名会失败。
- get_node_pins 对变量节点（VariableGet/Set）可能返回空，用完整 GUID 重试。
- 常用 spawner key：事件 EVENT CommonButtonBase::BP_OnHovered；函数用 discover_nodes 查。

**证据**：2026-08-14 实测（W_Cloth / W_ClothItemTooltip / W_Cloth_Item 蓝图操作）。

**时效性**：**版本相关**（VibeUE 5.0 BlueprintService）。

---

## 4. 蓝图变量：add_member_variable 创建的是只读 const 变量

**问题**：为什么 BlueprintService.add_member_variable 创建的变量在函数图里 Set 会报 read-only within this context？

**已验证结论**：
- add_member_variable 创建的蓝图变量默认是 **const（只读）**，函数图里 Set 节点编译报错。
- **解法**：用引擎原生工具 editor_toolset.toolsets.blueprint.BlueprintTools.add_object_variable（参数 blueprint/name/object_class），创建的是可写对象变量；或 add_variable（基本类型）。
- 变量类型在蓝图里显示为 object，连到 UWidget 引脚需 Cast 节点（或直接选对类型）。

**证据**：2026-08-14 实测（FocusTargetWidget 变量先 add_member_variable 报只读，改用 add_object_variable 后正常）。

**时效性**：**版本相关**（VibeUE / UE 5.8 BlueprintTools）。

---

## 5. 蓝图函数多分支返回：用 write_graph_dsl 的 (return ...)

**问题**：蓝图函数需要「A 有效返回 A，否则返回 B」的双 return 逻辑，MCP 怎么画？

**已验证结论**：
- 蓝图函数可含**多个 Return 节点**（每个 return 触发函数返回），但 VibeUE 建不了第二个 FunctionResult 节点（create_node_by_key("K2Node_FunctionResult|Add Return Node...") 静默失败）。
- **可靠做法**：用 BlueprintTools.write_graph_dsl 写函数图（**只对简单函数图**使用），DSL 支持 (return expr) 和 (Utilities|IsValid x (:"Is Valid" ...) (:"Is Not Valid" ...)) 多执行分支：
```
(fn GetDesiredFocusTarget ()
  (bind _item (Wardrobe|Focus|GetFirstItemEntryForFocus))
  (Utilities|IsValid _item
    (:"Is Valid" (return _item))
    (:"Is Not Valid" (return (Variables|W_Cloth|GetBackButton)))))
```
- write_graph_dsl 的 DSL 语法：用 get_graph_dsl_docs 查完整语法（event/fn/return/if/bind/switch 等）。

**证据**：2026-08-14 实测（BP_GetDesiredFocusTarget 最终用此方式实现）。

**时效性**：**版本相关**（VibeUE 5.0）。

---

## 6. 手柄焦点与输入：CommonUI 关键机制

**问题**：手柄焦点目标、返回键、按钮手柄触发分别怎么配？

**已验证结论**：
- **焦点目标策略放蓝图**：BP_GetDesiredFocusTarget（BlueprintImplementableEvent）在 W_Cloth 蓝图实现，C++ 只提供数据查询（GetFirstItemEntryForFocus 等 BlueprintPure）。改组件结构时只改蓝图。
- UCommonActivatableWidget::bIsBackHandler **默认 false**——不开的话 CommonUI Back 动作不响应。Back 键动作来自 ICommonInputModule 的 GetDefaultBackAction / EnhancedInputBackAction。
- UCommonButtonBase::TriggeringEnhancedInputAction（蓝图属性 triggering_enhanced_input_action）：给按钮配 InputAction 后，手柄按该动作直接触发按钮点击（无需焦点）。W_MenuButton 会自动显示按键图标（跨设备）。
- NativeOnHovered 设备无关：鼠标移入、手柄/键盘焦点导航、触屏按压都触发（引擎 CommonButtonBase.cpp 多处调用）。
- CommonUI 手柄焦点导航用左摇杆+DPad；DPad 被占用（配了 InputAction）后，网格导航走左摇杆即可。
- **CommonButtonBase 默认响应 A 键**（Confirm）作为点击——装备/卸下无需额外处理。

**证据**：2026-08-14 实测 + 引擎源码（CommonActivatableWidget.cpp、CommonButtonBase.cpp、UIActionRouterTypes.cpp）。

**时效性**：**稳定**（CommonUI 长期行为），个别 API 名 **版本相关**。

---

## 7. UI 职责边界：视觉/焦点/显隐放蓝图，C++ 只做数据与状态

**问题**：衣柜这类控件蓝图，视觉逻辑放哪层？改哪里才不会被坑？

**已验证结论**（本项目 ADR-0001 补充实践）：
- 焦点策略、悬停视觉、显隐切换、动画驱动 → **Widget Blueprint**（EventGraph / BP_* 事件）。
- 数据查询（当前页第一个格子、Tab 按钮等）→ **C++ BlueprintPure 函数**，但**不含策略**。
- 条目 hover/焦点反馈：EventOnHovered/EventOnUnhovered（CommonButtonBase 事件）→ SetVisibility(高亮Border)。参考 W_MenuButton 的 OnHovered 动画 + Ring/Glow。
- C++ 删掉旧实现时要留注释指向蓝图（如「策略在蓝图 BP_GetDesiredFocusTarget，C++ 只提供数据」）。
- 手柄键位：IMC 映射 InputAction → 按钮 TriggeringEnhancedInputAction，不写任何设备分支。

**证据**：2026-08-14 实测 + Docs/Engineering/ADR/Active/0001_UI蓝图与C++职责边界。

**时效性**：**稳定**（项目架构约定）。

---

## 8. MCP 资产操作注意

**问题**：MCP 读/写 UE 资产有哪些坑？

**已验证结论**：
- MCP「只读」蓝图工具（read_graph_dsl/GetWidgets 等）可能**重存资产**（Widget 蓝图重编译落盘），产生非预期 git M。读取前记录 LastWriteTime，读取后用「磁盘 SHA256 vs HEAD LFS oid」判断是否真改。
- 仅用 curl.exe 调 MCP（Python urllib/requests 会触发 UE HTTP 断言崩溃）。请求写无 BOM UTF-8 文件再 --data-binary @file。
- 编辑器内批量操作优先 execute_python_code（必须以 import unreal 开头）；单工具用 call_tool。
- ToolsetRegistry.get_all_toolset_json_schemas() 返回大 JSON 字符串，可解析找工具名/参数。

**证据**：2026-08-14 实测 + MCP_踩坑记录.md。

**时效性**：**版本相关**（UE 5.8.1 MCP / VibeUE 5.0）。

---

## 8.5 蓝图接增强输入事件（右摇杆等轴输入）的完整姿势

**问题**：W_Cloth 要接 IA_WardrobePreviewOrbit（右摇杆，AXIS2D）做每帧旋转，BlueprintService 怎么建事件节点？

**已验证结论**（2026-08-14 实测成功，commit 2ff7590）：
- **事件节点**：discover_nodes(bp, "Orbit") 找到 SPAWN K2Node_EnhancedInputAction|IA_WardrobePreviewOrbit，create_node_by_key 创建；输出引脚：Triggered/Started/Ongoing/Canceled/Completed（exec）+ ActionValue（struct）。
- **关键类型坑**：EnhancedInputAction 事件节点的 ActionValue 输出是 **Vector2D**（不是 InputActionValue）！所以用 FUNC KismetMathLibrary::BreakVector2D 拆 X/Y，不能用 BreakInputActionValue（类型不匹配连接返回 False）。
- **DeltaTime**：FUNC GameplayStatics::GetWorldDeltaSeconds 需要 WorldContextObject，接 FUNC Widget::GetOwningPlayer 的输出。
- **乘法**：UE 5.8 的 float 是 double，用 FUNC KismetMathLibrary::Multiply_DoubleDouble（搜 "float * float" 可得）；Multiply_FloatFloat 已不存在。
- 链路：Triggered(exec) → RotatePreviewByDelta.execute；ActionValue → BreakVector2D.InVec → X → MUL1.A；MUL1.B=速度常量（pin 默认值 90.0 度/秒）；MUL1.ReturnValue → MUL2.A；WorldDeltaSeconds.ReturnValue → MUL2.B；MUL2.ReturnValue → DeltaYawDegrees。
- 前置条件：W_Cloth 的 InputMapping 必须指向 IMC（UCommonActivatableWidget 激活时自动叠加，priority 500），否则事件收不到。
- 全部连接后 compile_blueprint + save_asset，再 get_node_pins 复读 is_connected 验证。

**证据**：2026-08-14 实测（W_Cloth 右摇杆旋转，编译通过已保存）。

**时效性**：**版本相关**（UE 5.8.1 / VibeUE 5.0 BlueprintService）。

---

## 9. 视觉模型（DeepSeekWeb2API）辅助

**问题**：非多模态模型怎么看截图/GIF？

**已验证结论**：
- 本地服务：http://127.0.0.1:3000/v1/chat/completions，API Key sk-local，模型 deepseek-vision / deepseek-vision-thinking。
- 配置在 G:\AI\DeepSeekWeb2API\config.json（server.host/port/apiKey）；使用前 curl /v1/models 查存活。
- 每次是全新会话无上下文，一次把背景写全。
- **已知限制**：对 GIF 只能看单帧（GIF 直发只返回第一帧）；抽关键帧再发更有效。

**证据**：2026-08-14 会话实测。

**时效性**：**可能过时**（DeepSeekWeb2API 为本地工具，行为可能随版本变化）。

---

## 10. 通用工作流提示

**已验证结论**：
- 修改前先 git log 查提交、读相关 Docs（AGENTS.md 强制流程），改后必须 commit + push（远程仓库是唯一沟通渠道）。
- 交互式/动态创建的条目（CreateEntry 生成）在蓝图中拿不到引用时，由 C++ 透传（BP_OnWardrobeItemHovered(Item, EntryWidget)）。
- 切页/重建面板后调 UCommonActivatableWidget::RequestRefreshFocus() 让 CommonUI 重新评估焦点（格子销毁后手柄焦点不丢）。
- 编辑器开着的状态下，GetWidgets/read_graph_dsl 后资产会变 M 属正常，判断用 SHA256 对比。
- 新会话接手：先读 AGENTS.md → 本文档 → MCP_踩坑记录.md → 相关任务包 → git log。

**证据**：本会话全程。

**时效性**：**稳定**（流程约定）。

---

## 11. 删除操作红线（2026-08-15 教训，务必遵守）

**问题**：Full access 权限下，AI 对批量文件/目录执行过强删，没有全程先停下与用户确认、未始终走回收站。

**结论（用户明确批评后定的规矩）**：
- 删除单个文件：仍按 AGENTS.md 先 Resolve-Path 验证路径以项目根开头，优先回收站（-Recycle 参数）。
- **删除多个文件或非空文件夹：一律列出清单 + 路径验证 + 预估影响，然后停下来等用户确认或让用户自己执行**。即使用户说"你来做"、给了 full access，也不自动动手——权限不是删除授权。
- 用户说"我来就行"意味着后续操作交还用户，AI 转为辅助（验证/检查）。
- 删除生成物（Build/Binaries/Intermediate/Saved/sln）前提醒：代码在 git 安全，但生成物删除本身仍要用户确认。
- 判断标准：**任何不可逆或批量操作，先问；可逆或单文件且路径验证通过，才可自行处理。**

**证据**：2026-08-15 全量清理目录事件，用户批评"删除是高危操作你有没有记住"。

**时效性**：**稳定**（协作规矩，不随版本变化）。