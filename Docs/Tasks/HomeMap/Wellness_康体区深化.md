# 康体区深化与轻量同步

## 2026-09-11 墙面、顶面与灯光收口

- 瑜伽室新增石材圆盘浅浮雕、木格栅、金属收边和墙顶灯槽；健身房与瑜伽室增加有木边与阴缝的顶面、两条线形灯和四个嵌灯。器械、瑜伽垫和中间活动区域保持，未修改玩法。
- 新增 SM_HM_YogaFeatureWall 为 3,784 三角面、SM_HM_WellnessCeilingDetail 为 2,464 三角面；普通 StaticMesh，前者保留实体碰撞，后者无碰撞。墙面主要部分内凸约 22 cm，墙顶檐口约 36 cm、位于 3.07 m 以上；最低顶面灯具约 3.06 m。新增模型沿用既有材质，仅灯具扩散面新增参数化 M_WellnessDiffuser，没有新贴图。
- 原两个主灯下移到 Z=301，避免被新吊顶包住。健身主灯 1750、瑜伽主灯 850，色温均 4200 K。新增一盏 200 lm、3800 K、225 cm 半径、无阴影的墙面 RectLight。原镜框灯半径从现场读到的 1000 cm 收回到 450 cm。
- 以上主灯、墙面灯使用 RectLightComponent。source_width/source_height 定义于 RectLightComponent；attenuation_radius 与 intensity_units 继承自 LocalLightComponent；temperature/specular_scale 继承自 LightComponent。健身主灯与镜框灯的 specular_scale=0，避免大面积辅助光源在镜中显示成不存在的矩形灯箱；实体线形灯网格负责可见发光形状。这是有意的美术取舍，不代表物理光源完全对应。
- 现有 M_Warm_Light 没有可调强度参数，所以只给新增两套网格引用 M_WellnessDiffuser，EmissionStrength=12；没有改全屋共享发光材质。
- Reports/wellness_envelope_walk.json 为新增几何后的真实角色连续行走，器械、两房通道、垫子与柜前 10/10 点通过，最终 Walking。后续调整仅涉及灯光和材质，未改变该路线几何。
- 9 月 11 日复读正式地图已引用两个新网格及扩散面材质，见 Reports/wellness_envelope_saved_state.json。Review/Yoga_Night_20260911.png、Gym_Night_20260911.png 是最终材质配置下运行时第 120 帧的视口实景；本机 game 窗口捕获失败，改用 editor 视口捕获，不包含 HUD。随后读回第 0 帧并退出 PIE，默认 Day 保持。
- 9 月 9 日 High 九视角性能归档不包含这批增量，也没有专门覆盖康体室镜面视角。当前不能将其当作最新版性能验收；本轮没有重复无隔离的整屋采样。

### 正式更新入口

- Blender：build_wellness_envelope.py，只更新本轮两个对象并保存 HomeMap_Master.blend；改造前本地副本为 Saved/HomeMapCheckpoints/HomeMap_BeforeWellnessEnvelope.blend。
- UE：先执行 create_wellness_diffuser.py，再执行 place_wellness_envelope.py；后者使用 wellness_envelope_manifest.json，复用 sync_static_kit.py。材质引用改变无需重导未变的 FBX。
- 关卡和 Master 曾先进入 e16d4a7e，配套新资源仍未跟踪；本次提交补齐两份网格、一份材质、FBX、清单、生产脚本与验证记录，修复该提交依赖缺口。下方早期章节保留当时制作记录，灯光数值以本节和实际关卡为准。

## 当前成果

- 正式关卡仍为 /Game/Environment/HomeMap/Maps/HomeMap_Courtyard，默认白天，16 m 泳池与 U 型住宅保持。
- 将 39 个器械、配重、瑜伽占位 Actor 替换为完整静态套件；增加运动地面、镜面木框、窄收纳柜、毛巾、卷垫、抱枕与收拢帘。保留原绿植、休息椅和边桌，修正边桌摆件悬空。
- 跑步机单件 5,288 三角面，复用两次；哑铃架 1,936；训练凳 1,740；瑜伽套件 1,028，复用两次；收纳柜 8,576。地面和镜框合为另一静态套件，具体统计在 SourceArt/HomeMap/wellness_manifest.json。
- 新器械使用普通 StaticMesh，Nanite 关闭；复用原橡胶、木材、金属、织物和塑料材质，没有新增贴图。开口器械使用 ComplexAsSimple 静态碰撞，瑜伽软垫与窗帘不阻挡角色。
- 只新增一盏镜框下 RectLight，Stationary、无动态阴影、影响半径 260 cm。原健身和瑜伽灯分别调至 2000 / 1350；镜面场景专用 M_Mirror 粗糙度从 0.075 调为 0.025，继续使用 Lumen，没有新增 PlanarReflection。

## 验证

- Reports/wellness_walk.json：既有角色实际连续走过器械前方、两房通道、柜前、瑜伽区，再回到健身入口，10 个路径点全部通过，终态为 MOVE_WALKING。
- Reports/wellness_sync_verification.json：重复同步 6 个网格全部跳过 FBX 重导，Actor 数量维持 478，没有重复实例。该计数对应本轮验证时刻，不是未来固定限制。
- Blender Master 已由 GUI 中的 Blender MCP 实际保存；六个套件经 BFEU 导出并在 UE 复查，未重导已经正确的楼梯和其他家具。
- 本轮没有重写 Character、Camera、Interaction、Experience 或语言系统，也不需要冷编译。环境性能仍以已有采样为参考，不能把本轮美术通行验证当作目标显卡达标证明。

## Nanite 材质保存

- 用户对 M_Kitchen_Ceramic 与 M_Courtyard_Stone 点 Fix 的处理正确；已复读 UMaterial 的 used_with_nanite 标记为 true，并确认保存。
- 进一步发现织物实例父级 M_Fabric 缺少标记，已补齐并保存。MaterialInstance 的 Nanite 支持来自其最终 UMaterial 父级，不能只保存实例而漏掉父材质。
- 衣柜现有模型的四个材质实例使用引擎 Interchange 的 M_Default 父材质。本轮最初的广泛检查误设置了该父材质的使用标记，已恢复为原 false 并保存；后续限定只修改 /Game 资产。
- 正式方案是 HomeMap 内的 M_HomeMap_ImportedSurface 与四个 MI_HomeMapVanity_* 实例，仅覆写 HM_Vanity_Interactive 的 StaticMeshComponent 材质槽。原共享衣柜模型、原四份材质实例和交互业务不变。无需继续给引擎插件 M_Default 点 Fix。
- 记录见 Reports/nanite_material_usage_verification.json；本轮为编辑器资产与保存检查，尚未执行独立打包验收。

## 可重复使用的最小流程

- BFEU 负责本地几何、Pivot、轴向、米到厘米转换与 FBX 导出；没有另造 Blender 导出器。
- SourceArt/HomeMap/sync_static_kit.py 读取显式清单。只处理列出的 StaticMesh 和同名 StaticMeshActor，拒绝覆盖其他类 Actor，也不会扫描或删除全场景内容。
- SourceArt/HomeMap/wellness_manifest.json 保存网格名、材质槽顺序、Nanite、碰撞及每个实例的 Location、Rotation、Scale。Rotation 顺序明确为 pitch / yaw / roll，UE Rotator 使用关键字构造。
- FBX SHA256 存在网格编辑器元数据 HomeMap.SourceSHA256；文件未变则跳过重新导入，仍同步清单中的实例位置和材质配置。FBX 改变后自动更新同一资产路径，保持实例引用。
- 需要调整某套件时，先保存 Master，再导出该对象并更新清单；不要重复执行一次性的建模脚本。仅在确认希望清单覆盖列出的实例变换时运行同步；UE 临时挪动不会自动反写 Blender。
- 重用入口在 UE execute_python_code 中执行，路径从项目根目录获取：

```python
import unreal
from pathlib import Path
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
exec(compile((root/'SourceArt/HomeMap/sync_static_kit.py').read_text(encoding='utf-8'), 'sync_static_kit.py', 'exec'))
sync_kit('wellness_manifest.json')
```

- 这是针对当前制作的静态套件同步入口，不承担蓝图迁移、Gameplay、全场景往返、资产删除或构建系统。它已经用于本轮六个网格和八个实例，后续有同类套件再复用。
