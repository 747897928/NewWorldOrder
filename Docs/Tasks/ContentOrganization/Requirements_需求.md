# 内容目录整理需求

## 目标

为 NewWorldOrder 建立一套可长期维护的 Unreal Content 目录标准。标准借鉴 Lyra 的功能域组织方式，同时适配本项目的 PVE、角色切换、CC 角色、武器、库存、CommonUI、HomeMap 和 Niagara 效果。

## 必须满足

- 目录名能够表达内容用途和所属系统。
- 生产资产、第三方源资产、测试资产和开发者实验资产分离。
- 美术表现资产与运行时玩法资产分离，但保持一个功能包的局部依赖可读性。
- Blueprint 不以“资产类型”作为唯一归类依据。
- 角色动画按骨架、角色归属和用途组织。
- `/Game/Assets` 有明确的迁出方案，不能只改名为另一个泛化目录。
- `ArchVizInteriorVol3` 有源资产保留策略和 HomeMap 生产资产策略。
- `NiagaraExamples` 有示例包隔离、使用资产提取和未使用资产处置策略。
- 所有迁移均通过 Unreal Editor 资产操作完成，不能直接在文件系统中移动 `.uasset` 或 `.umap`。
- 删除任何旧目录前必须确认没有蓝图、地图、DataAsset、材质、动画 Notify 或插件引用。
- 任务执行时必须在分批操作后提交 Git，保留可回退检查点。

## 目录语义要求

### 顶层功能域

推荐长期保留的生产域：

```text
/Game/AI
/Game/Audio
/Game/Characters
/Game/Effects
/Game/Environment
/Game/GameFramework
/Game/Gameplay
/Game/Input
/Game/Inventory
/Game/Maps
/Game/UI
/Game/Weapons
```

辅助域：

```text
/Game/Developer
/Game/ThirdParty
```

`/Game/CC_Shaders` 保持插件专属边界；`/Game/NiagaraExamples` 在未完成隔离前视为第三方示例边界。

### 资产包内部目录

新建或整理资产包时，优先使用以下语义目录：

```text
Meshes
Materials
Textures
Animations
Blueprints
Audio
Niagara
Data
```

只创建实际需要的子目录，不为了形式统一创建空目录。

## 不属于本任务的内容

- 不调整上游 Lyra 或第三方插件源码。
- 不改变资产的运行时父类、GameplayTag、ItemDefinition、AbilitySet 或组件职责。
- 不以目录整理为理由删除“看起来没有使用”的资产；删除必须有引用扫描证据。
- 不将所有材质、纹理、动画集中到全局类型目录。
