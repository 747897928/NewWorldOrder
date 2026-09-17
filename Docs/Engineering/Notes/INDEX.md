# 技术笔记索引

AI启动Session时必读本文件，快速定位需要的专家知识。

## 快速导航

按任务类型选择需要阅读的笔记：

UI相关任务：
- UI/CommonUI_架构与用法.md
- UI/RegisterUIActionBinding_学习笔记.md
- UI/UIExtension_HUD插槽学习笔记.md
- UI/Localization_Lyra语言切换调查.md

角色切换任务：
- CharacterSwitching/项目上下文.md

GAS相关任务：
- GAS/ASC_生命周期.md
- GAS/Weapon_命中流程.md
- GAS/TargetData_预测流程.md

武器相关任务：
- Weapons/Hitscan_命中实现.md
- Weapons/Projectile_投射物设计.md
- Weapons/Reticle_准星系统.md

网络同步任务：
- Network/Replication_最佳实践.md
- Network/ClientPrediction_客户端预测.md

UE5引擎问题：
- UE5/SaveGame_序列化规范.md
- UE5/SoftObjectPtr_软引用最佳实践.md
- UE5/CrossPlatform_跨平台注意事项.md

代码质量相关：
- Patterns/Sonar_代码质量约束.md
- Patterns/SSOT_单一数据源.md

## 按分类列表

### UI系统专家

UI-001: CommonUI架构与用法
- 文件：UI/CommonUI_架构与用法.md
- 要点：Lyra改进版CommonUI、GameplayTag驱动的层级系统、PushContentToLayerForPlayer、CommonUI+EnhancedInput长按、Widget激活时的Input Mapping Context切换
- 关键类：ULyraActivatableWidget、UCommonActivatableWidget、PrimaryGameLayout、AsyncAction_PushContentToLayerForPlayer
- 最佳实践：组件化UI管理、消息驱动更新、C++逻辑Blueprint表现、菜单长按走CommonUI Action Router而不是ASC输入链

UI-002: RegisterUIActionBinding学习笔记
- 文件：UI/RegisterUIActionBinding_学习笔记.md
- 要点：RegisterUIActionBinding 的作用范围、与 IA/IMC 的关系、为什么它不是全局监听、为什么当前项目的菜单独立切换键长按更适合走 Enhanced Input 手动 hold
- 关键类：UCommonUserWidget、FBindUIActionArgs、FUIActionBindingHandle
- 最佳实践：普通 UI 动作用 RegisterUIActionBinding，自定义独立键长按用 IA + IMC + C++ 状态机

UI-003: UIExtension HUD插槽学习笔记
- 文件：UI/UIExtension_HUD插槽学习笔记.md
- 要点：Lyra 的 W_ShooterHUDLayout / LAS_ShooterGame_StandardHUD 工作流、UUIExtensionPointWidget、UUIExtensionSubsystem、HUD.Slot.* 插槽、LocalPlayer 上下文注册
- 关键类：UUIExtensionPointWidget、UUIExtensionSubsystem、UGameFeatureAction_AddWidgets、UCommonUIExtensions
- 最佳实践：HUD layout 推到 UI.Layer.Game，稳定 HUD 片段通过 RegisterExtensionAsWidgetForContext 注册到 HUD.Slot.*；需要焦点和返回的菜单继续走 CommonUI 层级栈

UI-004: Lyra 语言切换与本地化调查
- 文件：UI/Localization_Lyra语言切换调查.md
- 要点：GameSettings 语言项、PendingCulture、Game 本地化目标、11 种首发 Culture、zh-Hans 首启默认策略、PO/locres 流水线
- 最佳实践：语言选项由编译后的 Game 本地化资源驱动；用户可见文字使用 FText/LOCTEXT；Lyra 和插件上游源码只读

### 角色切换专家

CHARACTER-001: 角色切换项目上下文
- 文件：CharacterSwitching/项目上下文.md
- 要点：实现状态总览、GameplayTag命名规范、AttributeSet架构、SaveGame结构
- 已实现：SaveGame系统、外观切换、AttributeSet、PlayerState性别标记
- 待实现：Attribute重命名、切换核心逻辑、技能系统集成、状态清理
- 重要警告：使用Abilities.Kit.Protagonist.Male/Female，不要使用Combat.*前缀

### UE5引擎专家

UE5-001: SaveGame序列化规范
- 文件：UE5/SaveGame_序列化规范.md
- 要点：SaveGame不能包含裸UObject指针，必须用软引用
- 常见错误：直接保存UTexture2D等指针导致跨平台崩溃

UE5-002: SoftObjectPtr软引用最佳实践
- 文件：UE5/SoftObjectPtr_软引用最佳实践.md
- 要点：TSoftObjectPtr使用、异步加载、IsValid判断
- 使用场景：SaveGame序列化、延迟加载资源

UE5-003: 跨平台注意事项
- 文件：UE5/CrossPlatform_跨平台注意事项.md
- 要点：未初始化内存在不同平台表现不同
- 常见陷阱：Windows上偶发正常，macOS必现崩溃

### GAS系统专家

GAS-001: Weapon命中流程
- 文件：GAS/Weapon_命中流程.md
- 要点：Hitscan vs Projectile的不同流程
- 架构：命中型走TargetData，投射物走Actor同步

GAS-002: TargetData预测流程
- 文件：GAS/TargetData_预测流程.md
- 要点：客户端预测、服务器验证、回传流程
- 对齐：遵循Lyra的预测窗口模式

### 武器系统专家

WEAPON-001: Hitscan命中实现
- 文件：Weapons/Hitscan_命中实现.md
- 要点：射线检测、散布、扩散恢复、多弹丸
- 类型：步枪、狙击、霰弹

WEAPON-002: Projectile投射物设计
- 文件：Weapons/Projectile_投射物设计.md
- 要点：爆炸伤害衰减、弱点倍率开关、服务器权威
- 类型：榴弹、火箭

WEAPON-003: Reticle准星系统
- 文件：Weapons/Reticle_准星系统.md
- 要点：散布角度计算、屏幕空间转换
- 基础：UShootReticleWidgetBase

### 设计模式与最佳实践

PATTERN-001: Sonar代码质量约束
- 文件：Patterns/Sonar_代码质量约束.md
- 要点：函数行数<=80、嵌套<=3、认知复杂度<=15
- 实践：提前返回、拆分辅助函数

PATTERN-002: SSOT单一数据源
- 文件：Patterns/SSOT_单一数据源.md
- 要点：每个知识点只在一处定义，其他地方引用
- 应用：SystemDesign为SSOT，QuickReference引用

## 笔记统计

总计：15个笔记
- UI专家：4个
- 角色切换专家：1个
- UE5专家：3个
- GAS专家：2个
- 武器专家：3个
- 设计模式：2个

最后更新：2026-09-06
