# 外围背景与屋顶休息区

## 本轮实际内容

- 正式关卡仍为 /Game/Environment/HomeMap/Maps/HomeMap_Courtyard，保存状态和进入游戏均为白天。既有 U 型房屋、双高度跳水路线、五米深水和玩法保持。
- 中央屋顶原临时石木长凳改为两组有软垫、木条靠背和扶手的座椅，补低茶几、书籍、杯碟、两只花池、木地板拼缝及提灯。茶几后缘与座椅前缘约留 1.2 m，跑向高跳台的中心区域保持开放。
- 原场地中心 100×100 m 保留；外侧补开口环状背景网格，内边采样原场地边界，外缘延伸到约 2.45 km。它只用于远景，不是新增可玩开放世界：无碰撞、无投影阴影，不参与距离场照明。屋顶向外现在有中景树带与远山；山体形态和层次仍偏简单，不能称为最终景观验收。
- 中景 72 棵树直接实例化已有 32,304 三角面的 SM_HM_CourtyardCanopyTree，复用现有 2K 材质。源几何总量为 2,325,888 三角面，实例化不会抹掉几何成本；设置 240 m 剔除距离、密度缩放和关闭动态阴影，性能须以本轮数据判断。
- 屋顶两盆植物、杯碟、书籍沿用原资产。没有新贴图；新增地形材质 M_EstateTerrain 使用既有 T_Soil_A、世界坐标噪声和坡度混合，默认单面。
- 新增一盏 Stationary、240 lm、3000 K 的局部提灯，半径 600 cm，无投影阴影，不改变曝光或昼夜业务。PointLightComponent 的 source_radius 定义于该类；intensity_units、attenuation_radius 继承自 LocalLightComponent，强度、温度和阴影设置继承自 LightComponentBase/LightComponent 链。无阴影会使近处桌板下方获得照明，是当前成本取舍，不能宣称物理完全准确。

## 源文件与更新入口

- 主源为 SourceArt/HomeMap/HomeMap_Master.blend；本地改造前检查点 Saved/HomeMapCheckpoints/HomeMap_BeforeLandscapeFinish.blend。正式 FBX 在 SourceArt/HomeMap/BFEU_Production/StaticMesh，全部已实际导入 UE。
- build_landscape_finish.py 更新背景、屋顶座椅、地板与花池，输出 landscape_finish_manifest.json 和 estate_tree_positions.json。四个最终网格分别为 EstateBackdrop 25,600、RooftopLounge 5,724、RooftopFinish 5,400、RooftopLantern 1,080 三角面；前三个由该脚本维护，灯具由 build_rooftop_lantern.py 维护。
- UE 先运行 configure_estate_material.py，再运行 place_landscape_finish.py；灯具独立运行 place_rooftop_lantern.py。后两者要求 PIE 已结束，重复同步使用 FBX SHA256 和树带位置摘要，避免重复导入与重复树实例。
- sync_static_kit.py 新增可选 destination 字段，背景进入 Landscape/Meshes；旧清单仍使用 Architecture/Hub。所有目的路径限制在 /Game/Environment/HomeMap 下。
- build_rooftop_route.py 记录之前的屋顶路线，但其中旧长凳会覆盖本轮软垫座椅。需要重做路线时必须随后重新执行本轮座椅构建与导入，不得将历史脚本当作当前完整重建入口。
- 开放地形首版因自动法线重算朝下，在 UE 出现黑色悬浮带；已在 Blender 明确检测和反转顶面法线，再导出、导入、截图确认。没有靠双面材质掩盖错误。
- 原树两个材质在首次用于本轮 Foliage 实例时由 UE 自动补 InstancedStaticMeshes usage，已保存并随本轮提交。这两份差异是实例化所需，不应当作无关脏资产丢弃。

## 验证及未完成范围

- Reports/landscape_lounge_assets.json 为本轮最终 UE 复读：正式地图、72 个树实例、四个网格的材质引用/碰撞/Mobility/真实包围盒、桌灯参数及默认 Day。它证明资产落地与设置，不代替视觉评审。
- Reports/rooftop_lounge_walk.json 使用真实项目角色，初始归位一次后连续走完桌后、座位前、花池旁及主通道，10/10 点通过，最终 Walking；没有修改 Character、Camera 或移动逻辑。
- 既有入口到屋顶、跳水、下潜与上岸记录见 RooftopRoute_屋顶观景与高跳台.md。本轮家具检查不代替全屋自由移动验收。
- Review/RooftopLounge_Day_20260909.png 为最终座椅和背景的编辑器实景；RooftopLounge_Night_20260909.png 为 PIE 第 120 帧夜晚的真实提灯效果。随后实际读回序列第 0 帧，并退出 PIE 保存白天。
- 本轮 High 采样包含全部新模型、树带和桌灯。采样期间还有其他三个 Unreal 项目运行，条件与上一轮不同，不能直接把差值归因于某个新模型；具体数据见 Performance_性能采样.md。
- 全屋最终风格一致性、远景美术品质、移动与夜间性能、Medium/Legacy 和目标显卡验证仍未完成。本记录是制作进展，不是整体完成声明。
