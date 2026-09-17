# HomeMap 制作状态

## 2026-09-12 外围林带深化

- 新增两种叶片式远景树、10 片林丛，共 340 个实例，补屋顶看到的中景层次。每种 5,244 三角面，总源几何约 178 万面；复用现有 2K 色图，不新增贴图、动态灯或可玩区域碰撞。
- Blender Master、正式 FBX、材质、FoliageType 与关卡已保存。首版球团树冠经实景检查淘汰，最终叶片版已导入；详细范围及重复执行规则见 Woodland_外围林带深化.md。
- 增量后 1080p High 十一视角实测 P95 为 10.771～12.845 ms，RTX 5060 Ti 本机结果；单帧峰值 49.347 ms。Medium/Legacy 旧结果不包含新林带，不能外推全场景或目标卡达标。
- 默认仍是白天。林带改善了中景空旷，远山与地表、全屋视觉统一、完整夜间/移动验收仍未完成；不因新增树木提高主观完成百分比。


## 2026-09-12 LegacyLow 镜面回退完成

- 根因已确认：`/Game/Environment/HomeMap/Materials/Hub/M_Mirror` 原本是 `Metallic=1、Roughness=0.025` 的纯金属材质，没有反射捕获或低档反射替代；关闭 Lumen 后环境反射退化为天空色，因此健身镜看起来像通向室外的洞。
- 未增加 PlanarReflection、反射捕获、动态灯或全局 Lumen。`M_Mirror` 增加 Quality Switch：Default 保留原镜面值供 High/Medium 使用；Low 使用中性深灰 `BaseColor=(0.16,0.18,0.19)`、`Roughness=0.32`、`Metallic=0` 的烟熏镜回退。该材质同时用于 `HM_Gym_Mirror` 与 `HM_Bath_Mirror`，两处低档都避免天空色洞感。
- `SourceArt/HomeMap/profile_pavilion.ps1` 显式设置 UE 材质质量枚举：LegacyLow=0、Medium=2、High=1，避免独立采样继承编辑器当前质量档。三档十一视角的既有性能归档未重跑。
- 在正式地图 PIE 中用同一 `CameraActor_9` 实际复查：LegacyLow 关闭 Nanite/VSM/Lumen、TAA 时镜面为稳定中性灰实体面，High 开启原 Lumen 路径时仍保留庭院/室内反射。截图见 `Review/LegacyLow_GymMirror_20260912.png` 与 `Review/Gym_High_20260912.png`，过程报告见 `Reports/legacy_mirror_fallback_20260912.json`。
- 复查后退出 PIE，正式关卡 `/Game/Environment/HomeMap/Maps/HomeMap_Courtyard` 已保存；`HM_DayNight_Control` 的 `InitialTimeOfDay` 仍为 `DAY`。本批没有改 Character、Camera、Interaction、Experience、UI、游泳或动画逻辑。
- 本批只完成低档镜面技术收尾，不代表 HomeMap 最终完成。整屋材质统一、远景、高处构图、完整夜间/移动、目标显卡和 Shipping 验收仍未完成，主观制作估计维持 70%～75%。

## 2026-09-12 三档采样与技术收尾交接（修复前记录）

- 最新 High/Medium/LegacyLow 已完成本机 1080p 白天十一固定视角采样，包含康体区最终增量；最差 Frame P95 分别为 16.050/13.989/12.030 ms。不是目标显卡、完整移动、夜间或 Shipping 通过，条件见 Performance_性能采样.md。
- LegacyLow 健身镜面退化成天空色，视觉未通过，失败截图已归档，交由 Luna 优先修复和复查。
- 本批关卡增量为两台康体区环境评审 CameraActor，没有新建模、没有改玩家镜头。美术制作估计仍约 70%～75%，不是最终验收比例。
- 收尾边界见 Handoff_Luna技术收尾.md；整屋视觉统一、远景、完整夜间和移动验收尚未完成。


## 2026-09-11 康体区深化与提交补齐

- 健身房和瑜伽室补齐顶面、灯具造型、石材圆盘背景与木格栅；2 个网格共 6,248 三角面，无新增贴图。调整主灯与镜面反射，新增一盏局部洗墙灯。最终夜景已复看，默认白天，见 Wellness_康体区深化.md。
- 新几何下既有角色连续通行 10/10 通过。关卡和 Master 已随 e16d4a7e 保存，本次补交当时遗漏的配套网格、材质、FBX、生产脚本、清单和报告。
- 主体布局、各功能空间、泳池和高处探索路线已落地；当前为整屋美术深化阶段，主观制作估计约 70%～75%，不是最终验收百分比。剩余整屋风格统一、远景与材质细化、完整移动/夜间以及分档性能验证。
- HomeMap 正式文件与临时文件分类见 GitOwnership_文件归属与提交.md。其他任务的角色、武器、动画和测试地图改动不混入本次提交，也不加入忽略规则。

## 2026-09-09 外围背景与屋顶休息区

- 屋顶临时长凳改为木条软垫座椅，补低茶几、杯碟书籍、两只花池、木地板拼缝和一盏局部暖光提灯。真实角色绕桌、经过座位和花池后回到主通道，10/10 路径点通过。
- 新增连续外围背景网格和 72 个既有树实例。保留原场地、建筑和水池；远景不设碰撞、不投影阴影。开放地形首版反面问题已从 Blender 法线修正，重新导入并截图检查。
- 白天座椅/背景与 PIE 夜间桌灯已实际截图，随后读回 Day 第 0 帧并保存。源文件、正式网格与材质均已落地，详见 LandscapeLounge_外围与屋顶休息区.md。
- 本轮 High 性能记录包含全部增量，九视角 P95 为 22.866～29.381 ms，未通过稳定 60 FPS；同时有其他三个 Unreal 项目运行，不能与此前单项目数据直接比较或归因。具体条件与结果见 Performance_性能采样.md。
- 尚未达到全屋最终验收：外围山体仍偏简单，整屋风格一致性和完整移动/夜间/分档性能仍需继续处理；不因家具和局部通行通过而上调验收百分比。

## 2026-09-08 屋顶观景与五米深水

- 补楼梯东侧墙窗与常开侧门、十处 Gallery/阳台护栏端柱。替换旧整片高窗碰撞，侧门可以实际通行。
- 保留二层低跳台；新增 22 级外楼梯到中央屋顶、木石长凳和独立东侧高跳台，离水约 7.62 m。两翼仍保持低矮，没有新增完整三层房间或 Gameplay 系统。
- 原浅水台阶保留，深水段加深到 5 m，原 PhysicsVolume 只向下扩展。入口至屋顶再高跳入水 14/14 路径点通过；继续下潜、返游并上岸 3/3 通过。详见 RooftopRoute_屋顶观景与高跳台.md。
- 当前版本已完成 High 九视角独立采样，本机 RTX 5060 Ti / i5-10400F、1080p 下九处 Frame P95 为 12.694～16.336 ms；包括本次全部增量。不是 RTX 3060/4060、夜间、移动或 Legacy 验证。
- 主线尚未完成：当前高处暴露了远景地形和天际线的简陋，屋顶陈设也未达到最终视觉质量。约 70% 仍只是粗略制作估计，不因新路线通过上调成验收进度。

## 2026-09-08 收纳深化与屋顶视觉纠正

- 正式关卡仍为 HomeMap_Courtyard，默认白天。增加衣帽抽屉、开放挂衣收纳、出征区木作和主卧顶面；现有衣帽交互 Actor 仅靠墙移动，未改业务。衣帽至卫浴、卧室的现有角色连续通行 11/11 通过。
- 替换外围树的离散大叶片，实际重建普通 LOD0 为 32,304 三角面，保留 12 个原树实例和既有 2K 贴图。
- 用户二楼截图指出初版碎石和矮墙仍是白板观感，该反馈成立。随后补深色压顶、六段种植床、26 个既有植物实例和检修分区，并压暗屋面材质。东、西窗截图已保存；碎石在强日光下仍偏浅，不以“已添加”宣称视觉验收通过。详情见 JoineryLandscape_收纳与外围深化.md。
- High 已新增有效八视角采样，但它在最后屋顶种植修改之前完成，四处 Frame P95 超过 16.67 ms。Medium 末段受编辑器恢复实时更新干扰，未作为有效结果发布；Legacy 尚未运行。详见 Performance_性能采样.md。
- 整体主观制作估计仍约 70%，不是验收百分比。剩余整屋风格一致性、外围背景、昼夜与移动实景、最终增量和目标硬件性能验证；本轮没有完成全部 HomeMap。

## 2026-09-08 建筑围合、开放楼梯与餐厨庭院深化

- 用户指出的五处不完整问题已实际处理：跳台补金属端柱、立柱与底座；清除楼梯前椅子、叠片玻璃、落地细竖框和矮灯；Gallery 后侧、西侧补实墙、窗洞和窗台；入户补双扇木门、雨棚、低花池与坐凳。默认常开，门交互尚未实现。
- 同批完成厨房收纳、中岛细节、餐区木顶、花园木廊架和休息家具、两翼檐口与静园踏石。模型实际导入正式 HomeMap，Master.blend 已保存，没有改 Gameplay 或新增动态灯。
- 现有角色从门外连续经过客厅、楼梯、Gallery、跳台并进入游泳，9/9 路径点通过；厨房到花园通行 14/14 通过。路径成功不代表全屋镜头与舒适度验收，具体画面与接口见 ArchitecturalClosure_入口与上层围合.md。
- 9 月 7 日跳台重复测试失败已定位为低频回调下满输入跨过目标、来回振荡。仅调整测试注入，重新实走 5/5 通过；最新 Reports/diving_terrace_walk.json 为成功结果，旧失败保留在 Git 历史。本页以下日期条目是当时状态，不应当作当前未修问题。
- 默认白天。日间太阳提高角度，夜间原月光/天空光/池灯调整；曝光范围 4.5～12 EV。新一轮有效 1920×1080 High 五相机采样见 Performance_性能采样.md，不再引用无效的 888×500 采样。
- 当前整体主观制作估计约 70%，尚未完成。剩余重点是整屋一致性、外围树冠与窗外背景、夜间和移动画面的质量/成本，以及 Medium、Legacy 和目标显卡验证；不得用局部路线通过宣布全屋完成。

## 2026-09-07 Pavilion 与直线跳台提交检查点

- 已保存并提交当前环境增量：中央挑高三环吊灯、Gallery 木石背景与浮雕、屋檐细节、复用窗帘，以及客厅茶几、地毯、座椅和摆件的尺寸与落位修正。客厅至 Gallery 的独立实走 11 个路径点通过，见 Reports/pavilion_walk.json。
- 跳台按用户实际试玩反馈改为阳台正面的直线通路，宽 180 cm、顶面 Z=352 cm，前端 Y=890；取消初版屋顶绕行和横向折返。两侧使用原建筑玻璃材质和细金属顶边，末端开放，未新增交互或按键。
- 深水池底改为 Z=-398 cm，水面仍为 -18 cm，深水 380 cm。保留原 11 级浅水台阶，Y=640..940 之间连续坡降；原 PhysicsVolume 仅把下边界扩展到 -450，XY 和水面不变。游泳已由 Luna 完成，当前说明见 Swimming_Implementation_实现说明.md。
- 首次完整跳台实测 5 个路径点通过，实际在约 (-282,1387,-35) 切换到 MOVE_Swimming；随后下潜至目标 -280 并游回浅水台阶、恢复 Walking，池底射线也命中设计高度。重复自动化测试在 Gallery 目标 (-450,-730) 附近未完成，最新失败报告保留在 Reports/diving_terrace_walk.json，不将本提交视为重复稳定通行验收。后续先检查该处行走与测试输入，不改游泳代码。
- 用户要求先提交，因此当前收口为可回退制作检查点；新一轮 1080p 多视角性能验收尚未完成。先前 Pavilion 采样被运行时覆盖成 888×500，已隔离到本地 Saved，不作为有效结果入库。Performance_性能采样.md 保留早期有效采样及其适用范围。
- 当前桌面默认抗锯齿从关闭改为 TSR，配置在 Config/DefaultEngine.ini；不是目标显卡性能通过声明。此修改影响项目默认渲染，Legacy 仍需独立验收。
- Master 为 SourceArt/HomeMap/HomeMap_Master.blend。Blender 本地改造前副本在 Saved/HomeMapCheckpoints；关卡检查点在 Maps/Checkpoints/HomeMap_BeforeDivingTerrace_20260907。关卡检查点引用共享资源，不能独立恢复被更新的地形网格，完整回退应使用 Git 对应版本和源文件。
- BFEU 的已安装副本本轮启用时报缺失 fbxio/io_scene_fbx_4_4，未修改插件源码。直线跳台与深水池使用 Blender 内置 FBX 导出，保持米制、Forward=-Z、Up=Y；导入包围盒与池底射线已核对。流程及后续注意事项见 PavilionDiving_中央空间与跳水台.md。

## 2026-09-07 家园地图迁移

- 正式家园地图为 `/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`，由 `Config/DefaultGame.ini` 的 `SessionReturnMap` 指向；玩家返回副本时进入该地图。`/Game/UI/Menu/WBP_SessionScreen` 的 `HostMapId` 也已同步到该地图，联机入口不会再把玩家送回旧 HomeMap。
- `/Game/Maps/HomeMap` 已退出正式运行入口，资产暂保留用于迁移回溯，后续删除前仍需完成引用检查。
- 旧 HomeMap 中的 `Debug_WardrobeOwnership_ClearAll`、`Debug_WardrobeOwnership_GrantAll`、`Debug_WardrobeSpawner_AutoOverlap`、`Debug_WardrobeSpawner_PressToInteract` 已迁移到 `/Game/Maps/TestMap_ListenServer` 与 `/Game/Maps/TestMap_SplitScreen`，新家园地图不放置开发调试入口。
- HomeMap_Courtyard 复用现有家园 M 菜单逻辑；运行时通过 `SessionReturnMap` 判断家园/副本状态，不需要复制一套 UMG。家园菜单包含 Online、Wardrobe、Settings、Quit Game 和 Return to Game。

## 2026-09-05 首轮保存检查点

- 当前地图：`/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`。新建筑和专用材质统一在同一 HomeMap 目录的 Architecture/Hub、Materials/Hub。
- 476 平方米 U 型单层基底、中央泳池、142 个 Blender 建筑构件已保存；68 种尺寸模块已导出 FBX 并导入 UE，尺寸复读通过，配置简单碰撞。
- `SourceArt/HomeMap/HomeMap_Master.blend` 已由 Blender 5.2 MCP 实际保存。
- 主要家具直接引用现有 ArchViz 的 Furniture、Props、Architecture 和 Materials。厨房的原模型 pivot 偏离几何中心，制作脚本按包围盒底部中心放置，禁止去改原模型 pivot。
- 已摆放现有衣柜与 ExpeditionTerminal 实例，GameMode / Experience / Character / Camera 未修改。实际 PIE 已成功生成项目角色。
- 第一轮角色 sweep 发现休闲椅、餐椅侵入动线；对应位置已修正，尚待最终复测。线性 sweep 不执行 CharacterMovement 的自动踏阶，卫浴和泳池台阶必须另做实际行走验证。
- 旋转构造改为显式 pitch/yaw/roll，避免 Unreal Python 的位置参数轴顺序导致家具倾倒。
- 已完成首轮分区灯光、Lumen PP、玻璃和水面；材质编译检查通过。这是 Early Pass，尚不能称为最终视觉验收。
- PIE 初步追踪已完成，报告在 Reports。工作站为 RTX 5060 Ti；采样包含编辑器与自动化开销，不能外推 3060/4060 的目标帧率。Stats 显示编辑器 RedrawViewports / StartFinalPostprocessSettings 及 Slate 有明显 CPU 开销。

## 2026-09-06 当前制作基准与进度

- 当前工作关卡为 `/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`，由用户将已验收 BackUp 重命名而来。此前通过 UE API 对比确认，备份已包含建筑、泳池、外围与灯光收尾；只合入玻璃通道与阅读区家具的 10 项差异，没有全量覆盖备份的 Gameplay 实例或配置。
- 当前属于主体空间完成后的美术深化阶段，尚未达到参考图的最终视觉完成度。中央挑高、Gallery、西侧廊和观景阳台已完成；一层布局和池壳保持原位。
- 本轮实际制作了住宅昼夜控制台、常开玻璃门的轨道和金属边框，并通过 BFEU 导出。统一规则见 BFEU_Pipeline_楼梯与交互试点.md。没有用新流程重导所有旧资产。
- 楼梯上行和下行均以现有 CharacterMovement 实走通过，Capsule 沿用项目配置，未改角色、镜头或移动逻辑。报告见 Reports/stair_walk_final.json 和 stair_walk_down_final.json。上行由 `(360,170,90)` 到约 `(360,-745,442)`，末态为 MOVE_WALKING。
- 控制台复用试点 BP 与现有 IA_Interact，已实际验证 60→120→0→60 循环；用户最新要求默认白天，已配置 InitialTimeOfDay=Day，并清除旧 Level Blueprint 的固定黄昏初始化路径。编辑器保存状态同步为白天。
- 单层水材质保留真实水深散射和吸收。修复重复运行旧 polish_scene.py 时清零所有 Constant3Vector、连带抹掉 ColorScaleBehindWater 的错误；水面波纹频率按米制 UV 调低。禁止以水下参数全零的镜面黑池作为最终效果。
- 环境日光、月光和实时捕获的天空光改为 Movable，以支持实际昼夜旋转和变化。建筑和固定家具继续使用 Static；无需给所有小灯开启动态阴影。
- 全局减面后的旧树冠过疏，本轮另建按叶片覆盖率分配几何的外围树冠：9,456 三角面、两种现有材质、无新增贴图。原树和源纹理保留，未删除。Nanite 的形状保持使用 UE 5.8 的 MeshNaniteSettings.shape_preservation，不沿用旧版 preserve_area 属性名。
- `SourceArt/HomeMap/HomeMap_Master.blend` 已实际保存。关卡检查点在 Maps/Checkpoints；旧正式版本现在保留为 HomeMap_Courtyard_demise。
- 新增两张池畔躺椅，单件 3,196 三角面，使用现有木材和织物材质，未新增贴图；靠种植带摆放，池沿侧约留 109 厘米净宽，并进行角色通行检查。
- 已完成一次独立进程 1080p High 固定相机采样，稳定段平均帧耗时 16.386 ms、GPU 10.025 ms、FrameTime P95 23.722 ms；尚不能宣称稳定 60 FPS。条件、启动卡顿、DrawCalls 和 GPU 分类见 Performance_性能采样.md。
- 最后复查降低建筑玻璃雾白感，并调整现有阳台、楼梯和庭院灯的覆盖范围。夜间序列使用 Moon=16、Sky=1.0；关卡最终恢复并保存为白天。默认白天经重新进入 PIE 验证为 Day / 第 0 帧，见 Reports/default_day_verification.json。
- 当时为预留游泳，将主池水深由约 102 cm 加深到 212 cm，原入口延长为 11 级踏步。池沿和水面高度不变，池下地形局部下沉，避免穿入池底。当时只验收几何通行；当前游泳已实现、深水已进一步加深，见本页最新条目与 Swimming_Implementation_实现说明.md。
- 主卧与卫浴完成一轮正式细化：床头木饰面和软包、几何窗帘、悬浮双台盆柜、金属龙头、镜框与毛巾架均由 Blender 建模并经 BFEU 实际导入。床、床头柜、阅读椅与小摆件继续复用原资产，床头方向调整到西侧背景墙。新增套件、材质实例和验证说明见 PrivateSuite_主卧卫浴制作.md。
- 本轮现有角色连续经过卧室、衣帽通道、卫浴并返回床前，全部 6 个路径点通过，记录为 Reports/private_suite_walk.json。没有修改 Character、Camera、Interaction 或衣柜业务。

## 当前继续任务

- 康体区已完成一轮深化：两台跑步机、双层哑铃架、训练凳、瑜伽抱枕套件、木饰面收纳柜、运动地面、镜框与收拢帘已替换或补充原占位；现有角色连续通行 10 个路径点全部通过。复用 BFEU，增加约 60 行的显式清单同步入口，6 个网格重复同步无重导、无重复实例。详见 Wellness_康体区深化.md。
- 已核对用户修复的陶瓷和庭院石材 Nanite 标记并保存，补齐织物父材质支持；衣柜引用的引擎父材质改为 HomeMap 内副本与实例覆盖，避免依赖本机插件资产改动。默认白天和原交互系统保持。

- 2026-09-06 本轮收尾：正式名称已统一，默认白天；泳池向花园延长 6 m，现水面 8.5×16 m、水深 2.12 m。同步延长池壳、池沿、步道和基础，移动远端廊架、种植与背景树，并补齐两侧草地基座。现有 CharacterMovement 到达新池远端后返回庭院通过，新增段池底与步道射线均命中设计表面，见 Reports/pool_extension_walk.json。
- 已导入实体参考板、出征门标识、收藏柜和 Gallery 书桌，替换对应占位架和桌脚，复用原摆件。实体参考板由用户明确保留，只作环境陈设；没有任务接受、奖励、进度、服务端任务系统或新 UI。制作与 Git 整理说明见 Handoff_正式关卡与工作区交接.md。

- 保留既有 U 型结构及泳池主体，按用户新视觉参考深化中央 Living Pavilion。
- 继续细化已落地的 Gallery、立面与室内陈设，高频功能留在一层。
- 完善外围树冠、背景和庭院休息区，并继续平衡白天、黄昏和夜晚的光照。
- 保留已完成的通行与性能验证记录；当前优先深化卧室、书房、卫浴等房间的陈设、材质和光照，再对完整场景集中复测性能。自建资源继续从源头控制成本。

## 已知限制

- 当前游泳已接通；本轮只调整环境几何和原水体边界，没有重写角色、动画、移动或联网代码。重复跳台和新入口路线已通过；完整移动场景、夜间及目标硬件性能仍需验证。
- 迁移可能留下 UE 重定向记录；资产本体已位于 Environment/HomeMap，不能当作两份新模型重复维护。
- 用户会同步在编辑器中查看和保存其他资产；`InteriorHome_BackUp` 非本任务创建，不能覆盖或清理。
