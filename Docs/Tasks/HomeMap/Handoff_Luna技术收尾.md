# HomeMap 技术收尾交接

## 当前基准

- 接手任务：GPT5.6Luna家园环境设计师，ID 01a0942a-9872-7463-9432-c91df73d18a6。
- 正式关卡 /Game/Environment/HomeMap/Maps/HomeMap_Courtyard；SourceArt/HomeMap/HomeMap_Master.blend 为正式源文件。默认白天，16 m 泳池、5 m 深水段、低跳台与屋顶高跳台已落地，游泳由既有系统负责。
- 先读 STATUS_制作状态.md、Performance_性能采样.md、GitOwnership_文件归属与提交.md、Wellness_康体区深化.md。旧增量脚本不是一键重建入口，禁止整批重跑覆盖当前关卡。
- 美术制作估计 70%～75%，不是最终验收比例。最新工作为三档固定视角采样与两台评审相机，无新模型。

## 第一批：低档镜面与局部验证

### 2026-09-12 完成记录

- 已确认根因：`M_Mirror` 原为纯金属镜面，LegacyLow 关闭 Lumen 后缺少可用反射，整面退化成天空色。
- 已在 UE 材质图中增加 Quality Switch。High/Medium 使用原 Default；Low 使用 `BaseColor=(0.16,0.18,0.19)`、`Roughness=0.32`、`Metallic=0` 的中性烟熏镜回退。没有新增 PlanarReflection、反射捕获或全局 Lumen。
- 同一 `CameraActor_9` 的实际 PIE 截图已通过：`Review/LegacyLow_GymMirror_20260912.png` 为低档回退结果，`Review/Gym_High_20260912.png` 为高档原反射结果。正式地图已保存，`HM_DayNight_Control.InitialTimeOfDay` 保持 DAY。
- `profile_pavilion.ps1` 已显式设置材质质量枚举，三档十一视角性能归档保持不变，没有因本批局部材质修复重复整套采样。结果记录见 `Reports/legacy_mirror_fallback_20260912.json`。

以下条目是本批原始验收要求，完成情况以上述记录为准。

1. 修复 LegacyLow 健身房镜面显示成天空色。失败实景见 Review/LegacyLow_GymMirrorFail_20260911.png，高档夜景参考 Review/Gym_Night_20260911.png。先查现有镜面材质、反射捕获及画质配置，再判断局部捕获或材质回退。根因尚未确定，不把猜测写成结论。
2. 保持 High/Medium 表现；不能靠新增实时 PlanarReflection 或全局开启 Lumen 解决 Legacy 问题。低档镜面允许降级，但不能像通向天空的洞。截图必须来自实际 UE，不以参数正确代替视觉验收。
3. 同位置复查 High 与 Legacy，确认默认白天并保存正式关卡；灯光变化时复查夜间。只有新增成本或可见回归才重测相关视角，不重复已有效的整套采样。
4. 更新状态与结果，提交推送本批，汇报 commit 与截图。若必须大范围渲染重构，留下具体证据和方案，不扩展成新系统。

## 后续分工

- Luna 可继续已有路线的通行、碰撞、默认昼夜、资产保存与分档验证，以及明确的摆件悬空、穿插、缺失小收边、材质标记和局部剔除修补。尺寸明确的小模型可以在原 Master 中增量制作，复用材质与清单。
- 仍需集中美术深化：整屋材质、明暗与陈设密度统一，夜间主景，外围地形、树带和高处观景构图。可能涉及局部建模，但没有重建主体或完整新增楼层的需求。这些并未完成，也不是纯验证工作。
- 先完成第一批技术收尾，再用真实全景和问题截图决定后续美术工作由谁继续，不以模型名称代替成果验收。保留 U 型布局、主泳池、Gallery、开放楼梯入口与探索路线。
- 不改 Character、Camera、Interaction、Experience、UI、游泳与动画逻辑。目标显卡、完整移动、完整夜间及 Shipping 尚未验收，不外推本机数据。

## 工具与提交边界

- UE 使用 MCP/Python，不解析 uasset 二进制。Blender 与静态套件同步规则见 Wellness 文档及 production_state.json；不为了小改新建 Pipeline。
- review_cameras.json 有 11 台环境 CameraActor；Gym/Yoga 为 CameraActor_9/10，不是玩家 Camera。profile_pavilion.ps1 的 CaptureReview 在稳定段后抓庭院和健身截图，避免读回污染采样。
- 与其他任务共享工作区，角色、火箭筒、动画与测试地图仍有暂存和未暂存修改。只提交本批明确 HomeMap 路径，用 git commit --only；禁止 git add .、全局 reset/stash/clean 或忽略他人资源。
- 正式关卡与依赖、Master、必要 FBX、脚本、清单和选定截图须提交；Saved、自动滚动备份与缓存沿用忽略规则。不能整体忽略 SourceArt 或 uasset。
- 每个短批次保存、提交、推送，不等额度耗尽；不要连续花数小时处理一个非阻塞小物件。此交接不代表主线完成。
