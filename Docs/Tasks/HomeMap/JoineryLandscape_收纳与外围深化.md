# 收纳与外围深化

## 已保存内容

- 衣帽间原两只整块柜与石板柜门替换为实际抽屉、开放层板、挂杆、衣架和有布褶的织物陈设。手包和坐凳复用项目已有资源，柜体占地保持原西墙范围。
- 既有 `HM_Vanity_Interactive` 只把 Actor 位置从 `(-1050,910,0)` 改为 `(-1050,980,0)`，靠近北侧隔墙，释放入门空间。蓝图、交互半径、Widget、换装业务均未改。其蓝图为 `BP_HomeMap_Dressing_Table_Set`；这次是关卡摆放调整，不是交互系统重构。
- 原坐凳及其碰撞代理一起移到 `(-1070,770)`。现有角色连续经过柜前、梳妆台入口、卫浴和卧室，11/11 路径点通过，见 Reports/joinery_walk.json。
- 出征准备区保留原参考板、桌面和出征入口，增加完整抽屉柜、背板与上层板。调整现有 Mission 灯朝向，让墙面和桌面获得直接照明。参考板仍是环境陈设，不承担任务接取或奖励系统。
- 主卧补木质顶面收边与阅读墙装饰。新增三套木作分别 5,832、2,268、1,728 三角面，均为普通 StaticMesh；沿用已有木、石、金属和织物材质。
- 两翼屋顶初版只有内退碎石和浅色矮边墙；用户的二楼截图证实这些内容在日光下仍融合成白色平板，初版不通过视觉验收。此前“二楼窗外不再是整片白板”的汇报不准确。
- 根据该视角重新制作屋顶：补深色金属压顶、六段低种植床和横向检修带，碎石底色乘数降到 0.1，保留原白色檐口。套件最终 5,616 三角面。碎石直接复用 NiagaraExamples 两张既有 2K 纹理，没有迁移示例或修改其材质。
- 屋顶种植复用 SM_Plant_1，共 26 个 Foliage 实例，比例 0.75～0.85，埋入原花盆，关闭动态阴影，裁剪距离 8,000 cm。原模型每件约 40,546 三角面，26 件完整几何约 105 万面；实例化不能等同于几何免费，这批增量尚未完成性能复测。
- 东、西窗实景见 Review/RoofEast_20260908.png 和 RoofWest_20260908.png。当前能辨识种植带和深色边界，但强日照下碎石仍偏浅；不能称为全屋视觉完成，也不能把截图当作玩家全程移动验收。
- 外围地面使用现有土壤纹理与低频颜色变化，替代纯色。新建 M_HomeGardenSurface、M_RoofAggregate 两份 HomeMap 材质；原材质和纹理资产保留。

## 树冠与实际构建设置

- 仍使用既有 2K 树叶图集，没有重新下载高模或增加贴图。叶片从原 30～47 cm 调整为 14～23 cm；增加连接到枝条的末梢和 4,608 片叶，替换随机散落的大叶片。
- 源网格 32,304 三角面；12 个原树实例沿用原位置、缩放与材质。本轮不将全部场景降低为低模。
- 检查发现仅写 `UStaticMesh::NaniteSettings` 并保存后，渲染资源仍可能沿用旧的简化回退；当时普通 LOD0 仅 5,784 面，而 Nanite 源数据为 32,304 面。稀疏叶片因此消失或膨胀成明显的大三角形。
- 当前树改用普通 StaticMesh，经 `UStaticMeshEditorSubsystem::SetNaniteSettings` 的 `apply_changes=True` 实际重建，读取 `get_num_triangles(0)` 确认为 32,304。保留所有新叶片，不能只看导出报告判断 UE 已更新。
- UE 5.8 的 `MeshNaniteSettings.shape_preservation` 是 `NaniteShapePreservation` 枚举，不是浮点数。普通树方案不依赖该设置；若后续重新启用 Nanite，必须重新比较叶冠覆盖率与实际成本。

## 生产与交接

- Master 已保存：SourceArt/HomeMap/HomeMap_Master.blend；改造前 Blender 副本在 Saved/HomeMapCheckpoints/HomeMap_BeforeCanopyAndJoinery.blend。
- 建模入口为 build_remaining_joinery.py、build_roof_finish.py 和更新后的 build_courtyard_tree.py。木作、屋顶使用 joinery_manifest.json、roof_finish_manifest.json 与已有 sync_static_kit.py；树维持 Landscape/Meshes 下原资产路径。
- finish_canopy_joinery.py 配置材质、树构建设置和相关 Actor；finish_joinery_source.py 移除 Master 与 architecture.json 中被替代的旧柜体。不要运行历史灰盒脚本把方块柜重新放回场景。
- 新材质的 WorldPosition→ComponentMask 连接使用空输入名，不能用字符串 Input。后者在当前 API 中返回失败且图表不会连上，会造成灰色棋盘格。两份新材质已修复连接、重新编译并在视口检查。
- 两张屋顶纹理引用位于 `/Game/NiagaraExamples/Gallery/Megascans/Surfaces/Gravel_Ground_xbnefjm/Medium/xbnefjm_tier_2/Textures`。后续整理示例目录时必须保留这两份依赖或经 UE 引用迁移，不能直接删除整个目录。
- 新增环境评审相机覆盖卧室、衣帽间和出征区，均是无自动接管行为的 CameraActor，不修改玩家 Camera。profile_pavilion.ps1 支持 ScalabilityTier=2/1/0；进入游戏后再次应用画质和分辨率，以避免用户设置覆盖测试条件。
- 性能结果以 Performance_性能采样.md 及实际 CSV 为准。工作站分档采样不能证明目标旧显卡已通过；整体 HomeMap 仍未完成最终视觉与移动场景验收。
