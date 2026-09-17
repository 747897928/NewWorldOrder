# 入口与上层建筑围合

## 2026-09-08 本轮实际改动

- 正式关卡仍为 `/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`，默认白天。U 型平面、主泳池和既有 Gameplay 保留。
- 跳台原先只有玻璃、上下细边，逆光时像悬空栏杆。现在两侧各四根 4.5 cm 金属立柱，带底座和玻璃夹具；最前端仍留开放跳出段。主体支柱和斜撑保留，直线通路宽 180 cm。
- 楼梯入口移除一把露台椅及其碰撞代理、四片叠放玻璃和旧门框实例。矮灯及对应光源移到花园边缘，两根 690 cm 高细竖框仅保留二层段；一层由原白色主框架和新顶梁承接。资产本体未删除。
- Gallery 后侧改为三处竖窗和实墙，西侧改为低墙、两处横窗、石材窗台与黑色框架。原沙发背景、书桌和收藏柜有真实背墙；面向泳池的正面保持通透。窗帘收至窗侧，西廊原 200 cm 宽度只被窗台局部占用约 18 cm。
- 入户新增木质双扇门、黑金属门框、门把手、外挑雨棚、低花池和坐凳。门扇默认向外打开，入口净宽约 252 cm，门槛仅 1 cm。植物沿用现有模型，盆体埋入花池。
- 同批还已完成厨房储物层板与中岛细节、餐区木顶、庭院木廊架及休息家具、两翼檐口和静园踏石。新增网格沿用项目内材质，无新增 4K 纹理和动态灯。
- 日间太阳改为较高角度，缩短树冠投进泳池的阴影；夜间月光、天空光和原两盏池灯重新调整。PostProcessVolume `HM_Look` 使用 `FPostProcessSettings` 的曝光上限 12 EV、下限 4.5 EV，解除日间亮部上限过低的问题；保留原昼夜交互。

## 门的后续交互接口

- 本轮只交付模型与常开摆放，没有实现门交互。
- 网格 `/Game/Environment/HomeMap/Architecture/Hub/SM_HM_EntryDoorLeaf`，单扇宽 125.5 cm、高 246 cm，轴心在铰链下端，本地 +X 为门宽。
- `HM_EntryDoor_Left`：位置 `(-126,-1008,1)`，关闭 Yaw=0，当前开启 Yaw=-100。
- `HM_EntryDoor_Right`：位置 `(126,-1008,1)`，关闭 Yaw=180，当前开启 Yaw=280。
- 两者目前是 `AStaticMeshActor`，其 `UStaticMeshComponent` 为 Static、BlockAll。接入现有交互系统时必须改为 Movable，再由既有权威交互/复制链驱动旋转，不能仅客户端播放；不增加专用按键。

## 保存与重建边界

- 主源已保存：`SourceArt/HomeMap/HomeMap_Master.blend`。新增 FBX 在 `BFEU_Production/StaticMesh`，实际导入到 `Architecture/Hub`。
- 三份显式新清单：`completion_manifest.json`、`architectural_closure_manifest.json`、`entry_landscape_manifest.json`。使用现有 `sync_static_kit.py` 导入，核对材质分段数量及顺序。小型新增网格均关闭 Nanite；需要碰撞的墙、门和平台用 Complex As Simple，纯装饰关闭碰撞。
- `finish_architectural_closure.py` 只处理列出的环境实例；`finish_architectural_closure_source.py` 同步 Master 和 architecture.json 中已被替代的旧玻璃及竖框。不要重跑旧 `build_sliding_door_detail.py` 恢复已撤掉的障碍。
- Blender 仍采用已验证的米制、Forward=-Z、Up=Y 内置 FBX；本轮没有修改 BFEU 插件或重导正确的其他资源。
- 本地改造前保存点在 `Saved/HomeMapCheckpoints/HomeMap_BeforeArchitecturalClosure.blend/.umap`。关卡副本引用共享资产，完整回退应恢复 Git 对应版本的 Level、网格与 Blender 文件。
- 复用家具的 UE 摆放以关卡及 Reports/completion_placement.json 为准；Master 不是现有 ArchViz 家具的完整镜像。

## 验证与当前完成度

- `Reports/architectural_closure_walk.json`：现有 BP_ShootCharacter 从门外连续穿过入口、客厅、楼梯、Gallery，再跳入泳池；9/9 路径点通过，末态 MOVE_SWIMMING，起点之后没有瞬移。
- `Reports/completion_walk.json`：厨房、餐区、池畔与花园休息区 14/14 路径点通过。
- 先前跳台重复测试的失败来自低频回调下满输入反复越过目标。本轮只在测试器靠近目标时降低注入量；重新实测 5/5 通过，见 Reports/diving_terrace_walk.json。没有修改角色速度、胶囊或游泳代码。
- 路线通过只证明测试路线可走，不能代替玩家对镜头、宽敞程度和视觉完成度的判断。截图见 Review/Closure_*.png；五相机独立性能记录及范围见 Performance_性能采样.md。
- 主线仍在美术深化阶段。本轮前约 65%，本轮后约 70% 为主观制作估计，不能当成验收比例。仍需整屋连续视觉检查，深化外围树冠和窗外背景、统一各房间完成度，并完成夜间/移动场景及 Medium、Legacy 的性能验收；不能宣布正式场景完成。
