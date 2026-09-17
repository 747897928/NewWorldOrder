# Blender For Unreal Engine：HomeMap 楼梯与交互试点

## 结论

本次使用 Blender 5.2.1 LTS、Blender For Unreal Engine 4.4.8 和 Unreal Engine 5.8.2 完成小规模试点。插件可用，而且对以下工作有实际帮助：

- 批量导出选定的 Static Mesh。
- 将 Blender 对象的 Location、Rotation、Scale 写成可复核的 UE Actor Transform。
- 统一 FBX 轴向和米到厘米的单位换算。
- 在导出前执行潜在错误检查。
- 为导入脚本和附加数据生成可追溯的输出。

它不能自动修正已经错误摆放的 UE Actor，也不能替代关卡中的可玩性检查。对于本次楼梯，真正解决问题的是统一 Transform 规则和楼梯碰撞设置；不能只看导入没有报错。

## 当前规则

后续模块化建筑优先使用“Static Mesh 保持本地几何，UE Actor 保存场景 Transform”的规则：

1. Blender 对象保持正确的场景 Location、Rotation、Scale。
2. BFEU 导出时对需要由 UE Actor 承担旋转的对象开启 `Rotate To Zero For Export`，不要把同一份对象旋转再次 Bake 进 Static Mesh。
3. UE 导入后用 BFEU 生成的 Transform 数据或经过核对的数值放置 Actor，不凭肉眼旋转补偿。
4. FBX 使用已验证的 `Forward=-Z`、`Up=Y` 轴向；Blender 使用米，UE 使用厘米，单位换算只发生一次。
5. 不混用“旋转已 Bake 到 Mesh 后再给 Actor 旋转”和“旋转由 Actor 保存”两套规则。
6. 楼梯几何不能依赖默认自动凸包作为可行走碰撞；本次楼梯使用 `Complex As Simple`，避免整段台阶被封成一块斜面。

BFEU 面板中的关键设置来自插件当前安装版本，不能照抄旧版本截图。每次升级 Blender 或插件后，应重新检查属性名和导出结果。

## BFEU 操作步骤

1. 先在 Blender GUI 中对源文件执行 Save As，保留独立试点副本。当前试点副本为：

   `SourceArt/HomeMap/HomeMap_Master_BFEU_GUI_StairClearance_20260906.blend`

2. 选择最小测试对象，不要一开始重导整个 HomeMap。本次对象为：

   `Gallery_Stair`

   `Gallery_Stair_Landing`

   `Stair_Rail_330`

   `Stair_Rail_530`

3. 在 BFEU 的 Object 设置中：

   - 导出类型使用 `export_self_only`。
   - 导出静态网格，关闭无关的骨骼、动画、相机、Spline 和 Alembic 导出。
   - 对四个对象开启 `Rotate To Zero For Export`，由 UE Actor 保存场景旋转。
   - `Gallery_Stair` 关闭自动生成碰撞，碰撞 Trace Flag 使用 `CTF_UseComplexAsSimple`。

4. 导出到 Blender 源文件旁的试点目录：

   `SourceArt/HomeMap/BFEU_Pilot_Exports_ActorTransform/StaticMesh`

   导入目标使用：

   `/Game/Environment/HomeMap/Architecture/Hub/BFEU_Pilot_ActorTransform`

5. 在 UE 中导入后检查 Static Mesh 的 Bounds、Collision 和 Actor Transform。不要把导入成功当作验证成功。

6. 验证 Blender 与 UE 的位置、旋转、比例、Pivot 以及楼梯和护栏相对关系。需要恢复 Blender 摆放时，使用 `bfeu_check_pilot.py` 生成的 Copy Object Transform 数据作为数值来源。

本次使用的可复核脚本位于：

- `SourceArt/HomeMap/bfeu_apply_home_map_rule.py`
- `SourceArt/HomeMap/bfeu_export_pilot_actor_transform.py`
- `SourceArt/HomeMap/bfeu_check_pilot.py`
- `SourceArt/HomeMap/bfeu_apply_stair_clearance.py`

其中 `bfeu_apply_home_map_rule.py` 只允许写入文件名为 `HomeMap_Master.blend` 的源文件；GUI 试点副本不要直接运行这个脚本，避免误触发源文件保护条件。需要对副本操作时，先复制脚本并修改文件名保护规则，再人工核对输出。

## 楼梯试点结果

本次没有重导主线 `/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`，只在备份关卡上验证：

`/Game/Environment/HomeMap/Maps/HomeMap_Courtyard_BackUp`

楼梯入口保留墙体，但将楼梯整体向左移动，扩大入口前方和右侧护栏到墙的空间。最终备份关卡中的关键位置为：

- `HM_Gallery_Stair`：`(360, -300, 176)`，旋转为零。
- `HM_Gallery_Stair_Landing`：`(360, -610, 340)`。
- `HM_Stair_Rail_330`：`(260, -300, 296)`，Roll 约 `+30.386`。
- `HM_Stair_Rail_530`：`(460, -300, 296)`，Roll 约 `+30.386`。
- 保留 `HM_Public_Partition_560`。
- 右侧护栏最大 X 约为 462.5，墙体内侧 X 约为 550，净空间约 87.5 cm。

碰撞验证结果：

- 默认自动凸包会把楼梯碰撞成不可正常行走的整体斜面。
- 关闭自动凸包并使用 `Complex As Simple` 后，角色沿楼梯中心线实际走到顶部 Landing。
- 测试角色 Capsule 半径约 34 cm、半高约 88 cm；PIE 中最终移动模式为 `MOVE_WALKING`，不是跳跃越过楼梯。

本次最终试点 Static Mesh 位于：

`/Game/Environment/HomeMap/Architecture/Hub/BFEU_Pilot_ActorTransform`

主线已有的 `SM_HM_GalleryStair` 和主线关卡没有被这次试点替换。HomeMap 范围内的重定向器检查结果为 0；后续迁移资产仍须在确认引用已更新后再删除未引用重定向器。

## 备份

在修改 Blender 源文件前已经保存：

- `SourceArt/HomeMap/HomeMap_Master_Before_BFEU_Pilot_20260906.blend`
- `SourceArt/HomeMap/HomeMap_Master_Before_Stair_Clearance_20260906.blend`
- `SourceArt/HomeMap/HomeMap_Master_BFEU_GUI_StairClearance_20260906.blend`

Blender GUI 进程保持打开，没有使用 `-b`，也没有通过每次启动后退出的方式运行试点。

## 备份关卡中的交互试点

### 昼夜切换

备份关卡新增：

- 蓝图：`/Game/Environment/HomeMap/Blueprints/BP_HomeMap_EnvironmentControl`
- 实例标签：`HM_DayNight_Control`
- 实例位置：`(650, -450, 0)`
- 关卡序列：`/Game/Environment/HomeMap/Cinematics/LS_HomeMap_DayNight`
- 初始状态：黄昏
- 端点帧：白天 0、黄昏 60、夜晚 120

交互只复用现有 `IA_Interact` 和 `UShootGA_Interaction_Environment`，循环为：

`白天 → 黄昏 → 夜晚 → 白天`

没有新增键盘 N、手柄 View，也没有新增 IMC。实际注入 `IA_Interact` 已验证端点顺序为 `60 → 120 → 0 → 60`。

### 门和衣柜提示

备份关卡中的衣柜使用副本蓝图：

`/Game/Environment/HomeMap/Blueprints/BP_HomeMap_Dressing_Table_Set`

没有修改共享的原始衣柜蓝图。门使用继承自 `AShootExpeditionTerminal` 的提示组件，并在备份关卡实例上设置 Widget。

提示组件现在按物件根部与玩家的水平距离计算显示范围，不再把抬高到角色视线位置的 Widget Z 偏移计入距离。因此角色不需要贴到物件上才会看到提示。

当前备份关卡实际核对到：

- 衣柜提示 `DisplayDistance=256`，这是用户在蓝图中手动确认的值，应保留。
- 门提示 `DisplayDistance=256`，提示位置已从 Z 180 调整到 Z 90。
- 衣柜组件的实际 `InteractionRange=160`，用于防止提示出现后仍能从更远距离触发。
- 衣柜和门的实例 Sphere 半径当前为 256；若后续要继续缩小实际触发范围，应在对应蓝图或关卡实例中单独调整，不要覆盖用户已经确认的提示显示距离。

这套提示实现为每个 LocalPlayer 创建本地 Widget 实例，保留 CommonUI/Enhanced Input 的设备图标查询路径，适用于未来本地多人扩展。

## 未在本次试点执行

- NiagaraExamples 迁移。
- `NS_TeslaCoil` 建筑化、自动索敌和点击丧尸逻辑。
- 用 Blender 重新建模昼夜交互物品。
- 将昼夜控制器、门、衣柜交互物品重新布局到最终主线区域。
- 主线 HomeMap 全量重导或大规模重构。

这些事项应在本试点结论确认后单独安排，不能因为 BFEU 已验证可用就自动重导全部资产。
