# 外围林带深化

## 本轮内容

- 屋顶实景显示原中景树带稀疏、重复，大片裸坡使住宅缺少环境包围感。本批沿保留的背景地形布置 10 片疏密不同的林丛，新增 340 个实例，均位于中心场地以外。
- 两种远景树由 Blender 制作，均为 5,244 三角面：12 段主干/分枝和 850 片有叶形轮廓的叶片。图集 UV 复用现有树叶，未新增贴图。远景叶片按屏幕覆盖率放大，不适合移到可近距离游览的庭院。
- 首版实心聚合冠层在 UE 中呈球团状，未作为最终结果保留；正式 FBX 和 Master 中已替换为叶片版。新树与近处已有树共存，没有重建 U 型房屋、泳池、屋顶路线或原场内地形。
- 两份专用材质各只采样原有一张 2K 色图；叶材质为双面 Opaque，叶轮廓由几何裁出，无透明卡片开销。树皮单面。不采样远景难以分辨的法线与 ARM，Roughness=0.93、Specular=0.12；颜色系数叶片 0.55、树皮 0.8。
- 340 个实例源几何总量 1,782,960 三角面，实例化不等于零成本。使用独立 FoliageType，比例 0.85～1.55，最远剔除 400 m，支持密度缩放；关闭投影阴影与距离场照明，不增加动态灯。

## 保存和重复执行

- 正式源文件 SourceArt/HomeMap/HomeMap_Master.blend 已保存，改造前副本为 Saved/HomeMapCheckpoints/HomeMap_BeforeWoodland.blend。两份正式 FBX 位于 SourceArt/HomeMap/BFEU_Production/StaticMesh。
- build_estate_woodland.py 只替换 HM_EstateWoodland_0/1 两个自有源对象，生成 estate_woodland_manifest.json 与 estate_woodland_positions.json，不执行历史整屋构建脚本。
- 本轮 Blender 没有运行中的 GUI 实例，使用已安装 Blender 5.2.1 的后台进程读取正式 Master、运行脚本并保存。factory-startup 只作用于该临时进程，避免无 GPU 界面的第三方插件初始化错误，不修改用户插件配置。
- UE 执行 place_estate_woodland.py，复用 sync_static_kit.py。脚本验证正式地图且 PIE 停止；FBX 摘要避免无变化重导，布局摘要避免重复叠加林带。改变点位文件时会停止并要求显式迁移，不能绕过断言再添加一批树。
- 材质资产属性定义于 UMaterial；cast_shadow、cast_dynamic_shadow、affect_distance_field_lighting、enable_density_scaling 由 UFoliageType 提供，UFoliageType_InstancedStaticMesh 使用继承属性。树根位置来自 Blender 背景网格 BVH 射线，并下埋 25 cm，不依赖 UE 无碰撞地形的射线。
- Reports/estate_woodland_saved_state.json 复读每种树 170 个实例、真实三角面及包围盒。第一次 MCP 导入调用超过 30 秒响应上限，但脚本已完成；通过报告和编辑器实际实例数确认完成后继续，没有重复启动或叠加实例。

## 验收范围

- 本批没有改变中心可玩区域和角色路线，不以旧通行报告宣称新增全屋自由移动验收。
- 完整游戏实景与本批增量性能结果见 Performance_性能采样.md 的最新条目。此前三档固定视角采样不包含新林带。
- 林带解决中景空旷的一部分问题；远山形态、地表细节与全屋夜间仍需深化，不将本批视作最终环境完成。
