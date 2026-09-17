# 内容目录设计规格

## 1. 总体目录标准

项目采用“功能域优先、资产类型辅助、第三方隔离”的规则。Blueprint、DataAsset、网格和材质首先归属于一个功能域，只有在功能域内部需要进一步降低查找成本时，才使用 `Meshes`、`Materials`、`Textures`、`Blueprints` 等子目录。

推荐目标树：

```text
/Game
├── AI
├── Audio
├── Characters
├── Effects
├── Environment
├── GameFramework
├── Gameplay
├── Input
├── Inventory
├── Maps
├── UI
├── Weapons
├── Developer
├── ThirdParty
└── CC_Shaders
```

不建议新增或扩展以下泛化生产目录：

```text
/Game/Assets
/Game/Blueprints
/Game/Misc
/Game/Temp
```

## 2. ArchVizInteriorVol3

### 原始源资产

原始资源包建议隔离为：

```text
/Game/ThirdParty/Epic/ArchVizInteriorVol3
├── Maps
├── Materials
├── Meshes
└── Textures
```

迁移前 `/Game/ArchVizInteriorVol3` 中的 `Interior.umap`、`InteriorHomeMap.umap` 和 `Overview.umap` 是资源包内地图。
用户已明确确认它们需要保留并使用，因此本轮将它们与其余资产一起迁入 HomeMap 生产目录；`ThirdParty/Epic/ArchVizInteriorVol3` 仅作为迁移前的来源边界，不再作为运行时资产容器。

### HomeMap 生产资产

实际为 HomeMap 服务的项目资产归入：

```text
/Game/Environment/HomeMap
├── Maps
├── Architecture
├── Furniture
├── Props
├── Materials
├── Textures
└── Blueprints
```

正式地图本体独立放在：

```text
/Game/Maps/HomeMap
```

本轮 ArchViz 资源包的全部 255 个资产均迁入上述生产目录并保留，其中 `Maps` 目录保存三张资源包地图；不因“演示”命名而删除或排除它们。
`/Game/Maps/HomeMap` 仍是项目正式地图入口；资源包地图是否作为正式入口、子关卡或制作参考由关卡设计另行决定。
当前目标目录的实际类型分布为：`Maps` 3、`Materials` 96、`Textures` 72、`Architecture` 19、`Furniture` 19、`Props` 46；本批没有 Blueprint 资产，因此不创建空的 `Blueprints` 资产目录。

## 3. `/Game/Assets` 特殊资产归属

### AmmoBox

AmmoBox 的视觉形态虽然类似箱子，但运行时语义是弹药补给物，不属于普通 Furniture。

```text
视觉模型：
/Game/Environment/Props/Gameplay/Ammo/AmmoBox
├── Meshes
├── Materials
└── Textures

未来交互 Actor：
/Game/Gameplay/Interactables/AmmoPickup

未来物品定义：
/Game/Inventory/Items/Ammo
```

模型、交互 Actor 和物品定义是三个不同职责。模型路径不应决定库存或弹药逻辑的实现位置。

### Dressing_Table_Set

Dressing_Table_Set 是家具模型，同时承担衣柜交互表现。模型归 Environment，交互归 Gameplay，不放入 UI 或 Mutable 目录。

```text
视觉模型：
/Game/Environment/Props/Furniture/Wardrobe/DressingTableSet
├── Meshes
├── Materials
└── Textures

未来交互蓝图：
/Game/Gameplay/Interactables/Wardrobe

角色预览相关资产：
/Game/Characters/Heroes/Customization/Preview
```

`/Game/UI/Mutable` 只保留 Mutable 预览系统和界面相关资产，不承载衣柜家具模型。

### FirstKitAid

FirstKitAid 是医疗补给物的模型，不是背包容器本身。建议归入 Gameplay Props 下的 Health 分类。

```text
视觉模型：
/Game/Environment/Props/Gameplay/Health/FirstAidKit
├── Meshes
├── Materials
└── Textures

未来拾取 Actor：
/Game/Gameplay/Interactables/HealthPickup

未来物品定义：
/Game/Inventory/Items/Medical/FirstAidKit
```

### 三个资产的最终建议

```text
AmmoBox
/Game/Environment/Props/Gameplay/Ammo/AmmoBox

Dressing_Table_Set
/Game/Environment/Props/Furniture/Wardrobe/DressingTableSet

FirstKitAid
/Game/Environment/Props/Gameplay/Health/FirstAidKit
```

## 4. 动画目录

用户计划使用的路径可以作为 MF 专属动画入口：

```text
/Game/Characters/Heroes/CC/MF/Animations
```

建议内部结构：

```text
/Game/Characters/Heroes/CC/MF/Animations
├── Locomotion
├── Combat
├── Weapons
├── Interaction
├── Traversal
├── Poses
├── Additive
├── Montages
├── BlendSpaces
└── AnimationBlueprints
```

MF/MM 共用的动画放在：

```text
/Game/Characters/Heroes/CC/Shared/Animations
```

只服务某把武器且与角色专属动画强绑定的资产，可以放在：

```text
/Game/Weapons/<WeaponName>/Animations
```

`PoseIcons` 属于 UI 展示资源，建议放在：

```text
/Game/UI/CharacterCustomization/PoseIcons
```

## 5. Blueprint 归属规则

Blueprint 的目录由运行时所有者和设计特性决定，不由 Blueprint 这个文件类型决定。

```text
角色蓝图：
/Game/Characters/...

敌人和 AI：
/Game/AI/...

武器蓝图：
/Game/Weapons/...

能力和技能：
/Game/GameFramework/Skills/...

世界交互物：
/Game/Gameplay/Interactables/...

UI Widget：
/Game/UI/...

地图专属蓝图：
/Game/Environment/<MapName>/Blueprints

开发者测试和调试：
/Game/Developer/<Domain>/...
```

当前 `/Game/Blueprints` 中的内容建议按以下方向拆分：

```text
AbilitySystem、Skills       → GameFramework/Skills 或 Gameplay/Abilities
Actor、Interaction           → Gameplay/Interactables 或 Gameplay/WorldActors
Animations                   → Characters 或 Weapons 的动画归属目录
Character、Player            → Characters
CommonUI、HUD                → UI
GameMode、GameState、System  → GameFramework
GameplayCues                 → Effects/GameplayCues 或对应功能域
GameplayCueNotifies          → Effects/GameplayCues
Input                        → Input
Mutable                      → UI/Mutable 或 Characters/Customization
Testing                      → Developer
```

其中具体资产仍需按照实际引用和运行时所有者逐项判断，不能仅凭当前旧目录名批量移动。

## 6. NiagaraExamples

官方示例建议隔离为：

```text
/Game/ThirdParty/Epic/NiagaraExamples
```

如果该路径由插件或导入流程管理、无法安全移动，则保留原路径，并在任务清单中标记为只读第三方内容。

实际使用的 Niagara 内容再按功能迁入：

```text
/Game/Effects/Niagara
├── Common
├── Weapons
├── Characters
├── Environment
├── Abilities
└── UI
```

例如：

```text
/Game/Effects/Niagara/Weapons/Rifle
/Game/Effects/Niagara/Abilities/RobotCompanion
/Game/Effects/Niagara/Environment/Explosions
```

迁移 Niagara System 时必须同时审查 Emitter、Module、Parameter Collection、Material、Material Instance、Texture 和其他引用。不能只移动一个 `NS_` 资产。

## 7. 命名和子目录约定

- 新建子目录使用英文，目录名使用 PascalCase 或稳定的功能短语，不使用空格和含义不明的缩写。
- 资产类型由现有项目前缀表达，例如 `SM_`、`M_`、`MI_`、`T_`、`BP_`、`AS_`、`AM_`、`ABP_`、`NS_`。
- 目录使用复数类型名，例如 `Meshes`、`Materials`、`Textures`、`Animations`、`Blueprints`。
- 不为了统一而修改已有资产名；资产重命名只有在名称会导致误用、且完成引用审计时才进行。
- 功能域内部允许保留一个局部资产包的完整依赖结构，避免全局 `Materials`、全局 `Textures` 和全局 `Meshes` 重新形成新的资产堆。
