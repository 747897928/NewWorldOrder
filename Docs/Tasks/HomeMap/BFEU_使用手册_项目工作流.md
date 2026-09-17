# Blender For Unreal Engine 项目使用手册

## 结论

当前项目已验证 Blender For Unreal Engine 4.4.8 可运行于 Blender 5.2.1 LTS 和 Unreal Engine 5.8.2。

它适合负责 Blender 到 UE 的导出规范、轴向转换、单位转换、Pivot、批量导出、碰撞选项、Import Script 和导出前检查。它不是完整的关卡迁移工具，也不会自动把新 Mesh 匹配到已有 UE Actor。

项目推荐使用混合流程：

- BFEU 负责 Blender 侧的导出契约和检查。
- Python 负责 UE 批量导入、资产匹配、Actor 摆放、引用修复和结果验证。

## 标准工作流

1. 在 Blender GUI 中对源文件执行 Save As，先保留独立副本。当前项目不使用 `-b` 运行 BFEU，也不在每次导出后退出 Blender。
2. 只选择最小测试对象，不要第一次就重导整个 HomeMap。
3. 检查 Object Origin、Local Transform、`matrix_world`、Parent/Child、Applied Rotation 和 Applied Scale。
4. 在 BFEU 中设置导出类型、目标目录、轴向、单位和碰撞规则。
5. 执行 Batch Export，并保留 Import Script、Transform 数据和 Additional Data。
6. 在 UE 导入后核对 Static Mesh Local Space 和 Actor Transform，不要凭肉眼旋转 Actor。
7. 用数值或截图核对位置、旋转、比例、Pivot、材质和碰撞。
8. 小规模验证通过后，才能用于后续模块化资产。

## 推荐设置

### 单位和轴向

- Blender 使用 Metric 米制。
- UE 使用厘米。
- FBX 使用 `Forward=-Z`、`Up=Y`。
- 单位换算只执行一次。
- 保持插件的标准 Axis Conversion，不要在 Blender、FBX、UE 三处重复补偿。

### Transform 和 Pivot

模块化建筑优先使用以下规则：

```text
Static Mesh 保存正确的本地几何和 Pivot
UE Actor 保存场景 Location / Rotation / Scale
同一份 Rotation 不能同时 Bake 到 Mesh 又应用到 Actor
```

对需要由 UE Actor 保存场景旋转的对象开启 `Rotate To Zero For Export`。BFEU 的 Copy Object Transform To Unreal 和 Additional Data 可以作为 UE Actor Transform 的数值来源。

### 导出和导入

- Selection Filter 使用 `only_object`。
- 模块化 Static Mesh 使用 `export_self_only`。
- 只打开需要的 Static Mesh 导出，关闭无关的骨骼、动画、相机、Spline 和 Alembic 导出。
- 需要自动生成 UE Import Script 时，开启 Text Import Asset Script。
- 需要保留复核数据时，开启 Text Additional Data。
- 导入目录通过 BFEU 的 Unreal Import Location 配置，不要默认接受 `/Game/ImportedBlenderAssets`。

当前 HomeMap 试点使用过的目标格式为：

```text
Game/Environment/HomeMap/Architecture/Hub/<PilotFolder>
```

### 碰撞

默认自动凸包适合普通封闭道具，但不适合所有建筑结构。

- 楼梯、台阶、需要逐面行走的结构：关闭自动凸包，按实际需求使用 Complex As Simple 或专用简单碰撞。
- 普通模块化墙体和装饰件：优先使用合理的简单碰撞，避免全场景依赖复杂碰撞。
- 碰撞设置必须在 UE 中实际做角色行走验证，不能只看导入成功。

## 试点中遇到的坑

### Double Transform

最容易出现的错误是：

1. Blender Object Rotation 已经 Bake 进 Static Mesh。
2. 又把 Blender Object Rotation 复制到 UE Actor。
3. UE 中得到双重旋转，护栏等对象的位置和角度明显错误。

解决方式是先决定旋转由谁负责。本项目模块化建筑默认让 UE Actor 保存场景旋转，BFEU 导出时使用 `Rotate To Zero For Export`。

### 默认导入目录不是资产归属规则

`/Game/ImportedBlenderAssets` 只是插件可以使用的导入目录，不代表项目最终资产目录。BFEU 可以设置目标路径，但它不会自动把导入资产迁移到已有目录，也不会自动替换旧 Actor。

需要迁移已有资产时，必须另外执行：

- 按命名或 Transform 匹配源对象和 UE 资产。
- 更新关卡 Actor 的 Static Mesh 引用。
- 验证材质、碰撞和 Actor Transform。
- 修复引用后再清理未引用重定向器。

### 轴向和单位重复转换

如果 Blender、FBX 导出器和 UE Import Transform 都各自做了一次轴向或单位补偿，结果会出现旋转轴错误、尺寸错误或位置偏移。遇到差异时，先记录三处设置，再判断是哪一步重复转换；不要直接在 UE 中手动旋转掩盖问题。

### 插件检查不等于可玩性检查

BFEU 的 Error Checking 可以发现一部分导出风险，但不能判断：

- 第三人称角色是否能进入楼梯。
- Camera 是否被墙体或护栏挤压。
- 交互提示是否容易看到。
- 角色是否被碰撞卡住。

这些必须在 UE 中用实际角色和实际关卡验证。

## BFEU 和 Python 的选择

### BFEU 更合适的情况

- 美术人员在 Blender 中选择对象并批量导出。
- 统一 Pivot、Axis、Unit 和导出选项。
- 生成 Import Script、Transform 数据和 Additional Data。
- 在导出前给出插件能够识别的潜在错误。

### Python 更合适的情况

- 批量导入几百个资产。
- 把源对象匹配到已有 UE Static Mesh 和 Actor。
- 批量恢复 Location、Rotation、Scale。
- 更新引用、清理重定向器和删除确认未引用资产。
- 自动检查 Bounds、Collision、材质、关卡实例和角色通行。
- 需要完全可重复的项目专用规则。

## 项目默认决策

不要把 BFEU 当成必须替换所有 Python 的新系统。后续使用时：

1. 用 BFEU 固定 Blender 到 UE 的导出规则。
2. 用 Python 编排 UE 侧导入和迁移。
3. 用最小对象集做试点。
4. 通过数值、截图和角色行走验证后再扩大范围。
5. 不要为了使用插件而重导已经正确的资产。

楼梯的具体位置、护栏变换和本次试点证据保存在单独报告中：

`Docs/Tasks/HomeMap/BFEU_Pipeline_楼梯与交互试点.md`

## 当前生产增量入口

- 康体区已实际采用 BFEU 导出加显式清单同步，入口为 SourceArt/HomeMap/sync_static_kit.py。重复同步会跳过未改变的 FBX，不增加重复 Actor。
- 使用方法、限制和验证见 Docs/Tasks/HomeMap/Wellness_康体区深化.md。该工具只处理清单列出的静态套件，不代替完整场景同步，不会覆盖 Gameplay Actor。
