# Lyra 语言切换与本地化调查

调查日期：2026-09-06

适用版本：Unreal Engine 5.8.1，NewWorldOrder

状态：语言系统闭环、首启自动识别策略和本次 C++ 固定文案英文源文本统一已实现；`/Game/UI` 固定用户文案已完成首轮统一；新 HomeMap_Courtyard PIE 已验证英语家园菜单与退出主菜单弹框；待真实游戏进程完成持久化验收

范围：设置页面语言选项、`FText` 本地化资源、语言切换事务、首发语言和打包配置。

本调查对照本机 LyraStarterGame 工程的源代码、配置和本地化目录；Lyra 与 Unreal Engine 源码只读，项目适配必须放在 `Source/NewWorldOrder` 或项目自己的配置/资产中。

## 1. 结论

### 1.1 可以沿用 Lyra，但语言设置项本身不是翻译系统

项目当前已经有一条基本等同于 Lyra 的语言设置链路：

```text
W_LyraSettingScreen
  -> ULyraGameSettingRegistry
  -> ULyraGameSettingRegistry_Gameplay
  -> ULyraSettingValueDiscrete_Language
  -> ULyraSettingsShared::SetPendingCulture / ResetToDefaultCulture
  -> ULyraSettingsShared::ApplySettings
  -> FInternationalization::SetCurrentCulture
  -> GGameUserSettingsIni 中的 [Internationalization] Culture
```

这条链路只负责让玩家选择 Culture、保存待应用值、应用 Culture 和刷新设置状态。真正的翻译来自 `Game` 本地化目标生成的 `.locres` 资源，以及打包时被放入产品的 Culture 目录。只复制或修改设置页面 C++，不会自动产生中文、英语或其他语言的翻译。

### 1.2 当前项目的主要缺口

- `ULyraSettingValueDiscrete_Language` 已经存在，逻辑基本是 Lyra 原版的复制品。
- Gameplay 设置注册表和设置页面已经把语言设置项接入现有 CommonUI 设置框架。
- `Config/DefaultGame.ini` 已显式配置 11 个首发 Culture 的 `CulturesToStage`。
- `Config/Localization/Game_Gather.ini`、`Game_Export.ini`、`Game_Import.ini` 和 `Game_Compile.ini` 已建立，`Content/Localization/Game` 已生成 manifest、archive、PO 和 locres。
- 无玩家 `Culture` 覆盖时，正式游戏运行时按 Steam 应用语言、系统语言、`zh-Hans` 回退顺序选择 Culture；`Config/DefaultGameUserSettings.ini` 不预写 Culture，不依赖编辑器安装路径。
- 现有 C++ 用户可见文字同时存在 `LOCTEXT`、`NSLOCTEXT` 和固定文本 `FText::FromString`；本次已统一 `Private/UI`、`Private/Online`、`Private/Testing`、`Private/Interaction`、`Private/Pickups`、`Private/System`、`Private/Player` 范围内的固定本地化源文案，并清理明显的测试/POC 玩家措辞，日志和开发调试文字仍按非用户可见边界保留。
- `/Game/UI` 已扫描 87 个 Widget Blueprint，针对固定用户可见的 `Text`、`ButtonText`、`ExperienceDisplayName` 和 `PreviewText` 属性完成 104 处稳定 `NSLOCTEXT` 键整理，涉及 32 个资产；设计时样例、动态占位符、纯数字/符号和产品品牌字样保留原状。
- 项目已有 Noto Sans 简中、繁中、日文、韩文、阿拉伯文和基础字体资产，为多语言字体回退提供了基础，但仍需在编辑器和打包版本逐语言验证字形。

### 1.3 首发语言和默认语言必须分开定义

建议把以下 11 个 Culture 作为首发翻译资源集合：

```text
zh-Hans, en, ru, es, pt-BR, de, ja, fr, pl, ko, zh-Hant
```

其中 `zh-Hans` 是无法从 Steam 或系统语言识别到首发 Culture 时的首启回退显示语言，`en` 是本地化源文本和 `NativeCulture`。Lyra 的原始实现把空 Culture 名称插入第一个选项，含义是 `System Default`；它是额外的便利选项，不计入 11 个首发翻译 Culture。编辑器 PIE 仍可能沿用编辑器自身 Culture，必须用真实游戏进程或清洁用户配置验证首启策略，不能用编辑器当前语言反推打包结果。

当前运行时策略：

- 没有 `[Internationalization] Culture` 时，`UShootGameInstance` 调用项目层 Culture 解析器，依次匹配 Steam 应用语言、系统语言，最后回退 `zh-Hans`；自动解析只作用于当前进程，不写入用户配置。
- 玩家在设置页 Apply 后明确选择的 Culture 仍写入 `GameUserSettings.ini` 并覆盖自动识别结果。
- 当前设置页保留 Lyra 的 `System Default` 作为第 12 个便利选项；正式游戏清除玩家覆盖后重新执行上述自动识别，编辑器中继续保留系统默认语义。

## 2. 首发语言方案

下表中的 Steam 占比沿用本次需求提供的数据，作为市场排序参考，不作为本地化完成度或用户数量的精确统计。

| 建议显示顺序 | 语言 | Culture | Steam 占比参考 | 产品策略 |
| --- | --- | --- | ---: | --- |
| 1 | 简体中文 | `zh-Hans` | 22.52% | 首启默认语言 |
| 2 | 英语 | `en` | 39.61% | 基础语言，第二个显示选项 |
| 3 | 俄语 | `ru` | 9.30% | S 级，强烈建议首发 |
| 4 | 西班牙语（西班牙） | `es` | 4.91% | S 级；`es` 作为通用西语，需确认文案使用西班牙语口径 |
| 5 | 葡萄牙语（巴西） | `pt-BR` | 4.38% | S 级，使用巴西地区代码 |
| 6 | 德语 | `de` | 2.82% | A 级 |
| 7 | 日语 | `ja` | 2.43% | A 级 |
| 8 | 法语 | `fr` | 2.33% | A 级 |
| 9 | 波兰语 | `pl` | 1.74% | A 级 |
| 10 | 韩语 | `ko` | 1.45% | A 级 |
| 11 | 繁体中文 | `zh-Hant` | 1.33% | A 级；脚本级繁中，需在 UE 5.8.1 打包验证 |

代码注意事项：

- `es` 是 Lyra 和 Unreal 本地化配置中使用的通用西班牙语代码；`es-419` 通常表示拉丁美洲西语，本首发方案不包含 `es-419`。
- `zh-Hant` 表示繁体中文脚本，比直接使用 `zh-TW` 更适合作为脚本级资源目标；是否被当前 UE 5.8.1 的运行时和打包链路完整接受，必须通过实际 Gather、Compile、Cook 验证。
- `NativeCulture` 是翻译源语言，不等于玩家首启语言。即使本地化目标继续使用 `NativeCulture=en`，也可以把玩家首启 Culture 设置为 `zh-Hans`。

## 3. Lyra 的实际实现

### 3.1 语言设置项如何注册

Lyra 在 `Source/LyraGame/Settings/LyraGameSettingRegistry_Gameplay.cpp` 的 Gameplay 设置集合中创建语言设置：

1. 创建 `ULyraSettingValueDiscrete_Language`。
2. 设置开发名 `Language`、显示名 `Language` 和描述文字。
3. 使用 `FWhenPlayingAsPrimaryPlayer`，只让主本地玩家修改这类全局语言设置。
4. 把设置项放入 `Language` 集合，再由 Gameplay 页面显示。

NewWorldOrder 的对应文件为 `Source/NewWorldOrder/Private/Settings/LyraGameSettingRegistry_Gameplay.cpp`，当前调用链已经存在，不需要为了加入语言切换重新搭建页面。

### 3.2 语言选项从哪里来

`ULyraSettingValueDiscrete_Language::OnInitialized()` 调用：

```cpp
FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game)
```

这个函数返回的是当前 `Game` 本地化目标已经有本地化资源数据的 Culture 名称。随后 Lyra 还会用 `FInternationalization::Get().IsCultureAllowed()` 过滤不允许的 Culture。

因此，`CulturesToStage` 只负责把资源带进打包结果，并不会凭空生成翻译；如果没有 `Game` 目标编译出的 `.locres`，语言设置项无法列出项目真正支持的语言。

Lyra 接着执行以下处理：

- 在索引 0 插入空 Culture，显示为 `System Default ({系统默认名称})`。
- 对真实 Culture 使用 `GetDisplayName()` 和 `GetNativeName()` 显示本地名称与当前语言名称。
- 读取当前 Culture 时先精确匹配；如果当前是 `en-US`、`zh-CN` 这类区域变体，则使用 `GetPrioritizedCultureNames()` 回退到可用的 `en` 或 `zh-Hans` 等父级 Culture。

这意味着语言列表默认是“资源驱动”的，不是当前 C++ 中写死的 11 行数组。若产品需要固定的显示顺序或别名，应在项目层增加配置型语言目录/顺序，并且不要在 C++ 构造函数中硬编码超过 3 条的目录数据。

### 3.3 选择、应用、取消的生命周期

Lyra 使用 GameSettings 的事务模型，选择语言时先写入待处理状态：

```text
玩家选择语言
  -> SetPendingCulture(CultureName)
  -> 设置注册表变脏
  -> 页面显示 Apply / Cancel

Apply
  -> Registry::SaveChanges
  -> SharedSettings::ApplySettings
  -> SetCurrentCulture
  -> 写入 GGameUserSettingsIni
  -> 清除 PendingCulture
  -> 弹出重启提示

Cancel / RestoreToInitial
  -> ClearPendingCulture
  -> 恢复应用前的 Culture
```

当前项目的 `Source/NewWorldOrder/Private/Settings/CustomSettings/LyraSettingValueDiscrete_Language.cpp` 与 Lyra 对应实现基本一致：

- `SetDiscreteOptionByIndex()` 只设置 pending Culture 或待恢复系统默认标记。
- `OnApply()` 弹出 `Language Changed` 确认框，并提示完整重启游戏。
- `ResetToDefault()` 选择索引 0。
- `RestoreToInitial()` 调用 `ClearPendingCulture()`。
- `StoreInitial()` 当前仍是与 Lyra 相同的空实现，不应在没有测试依据时单独改动。

### 3.4 Culture 实际保存在哪里

这里需要纠正“SharedSettings 负责保存语言”的表面印象。

`ULyraSettingsShared` 继承自 `ULocalPlayerSaveGame`，但当前 Lyra 语言设置的实际持久化位置是：

```ini
[Internationalization]
Culture=zh-Hans
```

写入文件是 `GGameUserSettingsIni`，通常对应当前用户的 GameUserSettings 配置文件，而不是账号 SaveGame 中的角色或物品数据。

实际行为是：

- `SetPendingCulture()` 只设置内存中的待应用 Culture，并将设置标记为脏。
- `ApplyCultureSettings()` 调用 `FInternationalization::SetCurrentCulture()`。
- 应用成功后，把 Culture 写入 `GGameUserSettingsIni` 并 Flush。
- 选择系统默认时移除 `Culture` 键；下次由系统默认 Culture 决定。
- `OnCultureChanged()` 会清掉 pending 状态，避免 Culture 已经改变后设置事务仍保留旧值。

语言本身是进程级的国际化状态，而设置页面仍然是某个 LocalPlayer 的 CommonUI 页面。未来本地分屏时应继续让页面绑定明确的目标 LocalPlayer，并只允许主本地玩家编辑全局语言；不能为每个分屏玩家各自应用不同的进程级 Culture。

## 4. NewWorldOrder 当前审查结果

| 检查项 | 当前状态 | 证据 | 影响 |
| --- | --- | --- | --- |
| 语言设置 C++ | 已有 | `Source/NewWorldOrder/Private/Settings/CustomSettings/LyraSettingValueDiscrete_Language.cpp` | 可以沿用 Lyra 的事务和显示逻辑 |
| 语言设置注册 | 已有 | `Source/NewWorldOrder/Private/Settings/LyraGameSettingRegistry_Gameplay.cpp` | 不需要新增页面容器 |
| 设置页面 | 已有 | `Source/NewWorldOrder/Private/UI/Settings/LyraSettingScreen.cpp`、`/Game/UI/Settings/W_LyraSettingScreen` | Apply、Cancel、Reset 和 CommonUI 生命周期已有入口 |
| Game Culture 打包 | 已配置 | `Config/DefaultGame.ini` 显式包含 11 个首发 Culture | Cook/Package 时携带 11 个首发语言资源 |
| Localization Dashboard 配置 | 已建立 | `Config/Localization/Game_Gather.ini`、`Game_Export.ini`、`Game_Import.ini`、`Game_Compile.ini` | 可重复执行 Gather、Export、Import、Compile |
| Game 翻译资源 | 首轮已生成 | `Content/Localization/Game` 包含 11 个 Culture 的 manifest、archive、PO、locres | 语言设置可从 Game locres 发现 11 个首发语言 |
| 首启默认语言 | 已实现 | `UShootGameInstance::Init` 和 `ULyraSettingsShared::ResolveDefaultCulture` 按 Steam、系统、`zh-Hans` 顺序解析；`DefaultGameUserSettings.ini` 不预写 Culture | 非编辑器游戏首次运行优先使用 Steam/系统语言，无法匹配时简体中文 |
| 用户可见文本规范 | `/Game/UI` 与本次 C++ 范围已完成首轮 | 扫描 87 个 Widget Blueprint，104 处固定属性已改为稳定 `NSLOCTEXT` 键；`Private/UI`、`Private/Online`、`Private/Testing`、`Private/Interaction`、`Private/Pickups`、`Private/System`、`Private/Player` 固定本地化源文案已统一为英文，明显测试/POC 玩家措辞已改为产品措辞；动态/设计时文本按边界排除 | 后续按页面验证剩余动态文本、字体和真实运行时布局 |
| 字体基础 | 部分具备 | `Content/UI/Foundation/Fonts/NotoSans` 下已有 SC、TC、JP、KR 等字体资产 | 还需要配置 Composite Font/Fallback，并验证西里尔字母、重音拉丁字母和 CJK |
| 上游可修改性 | 只读 | Lyra/插件源码是上游参考 | 需要在 `Source/NewWorldOrder` 做项目差异，不修改 Lyra 源码 |

本次补充审计已处理上述固定用户文案：`ShootPlayerController.cpp` 的姿势/角色切换结果、两个角色切换入口以及 `ShootReviveInteractableComponent.cpp` 的救援交互均已改为稳定 `LOCTEXT`。日志、调试错误和只用于开发者的字符串仍需保持开发者文本边界，不要为了本地化把所有 `FString` 都机械替换。

## 5. Lyra 的本地化资源流水线

Lyra 的关键目录和文件如下；这些路径均相对于 Lyra 工程或 NewWorldOrder 工程根目录：

```text
Config/Localization/Game_Gather.ini
Config/Localization/Game_Compile.ini
Config/Localization/Game_Export.ini
Content/Localization/Game/
  Game.manifest
  Game.archive
  Game.locmeta
  en/Game.po
  en/Game.locres
  zh-Hans/Game.po
  zh-Hans/Game.locres
  ...
```

典型流程：

```text
C++ LOCTEXT/NSLOCTEXT + 蓝图 FText + DataAsset FText
  -> Localization Dashboard 创建 Game Target
  -> Gather Text From Source / Assets
  -> 生成 manifest、archive
  -> 导出 PO 给翻译
  -> Import 导回翻译后的 PO
  -> Compile 生成各 Culture 的 Game.locres
  -> DefaultGame.ini 配置 CulturesToStage
  -> Cook / Package
  -> 运行时由 TextLocalizationManager 加载 Game locres
```

### 5.1 配置文件职责

- `Game_Gather.ini`：定义源代码、Config、Plugins、Content 资产的 Gather 范围和 manifest/archive 输出。
- `Game_Export.ini`：把已 Gather 的文本导出为翻译人员使用的 PO，并保留注释和源位置。
- `Game_Import.ini`：把翻译人员交回的 PO 导回 archive；修改 PO 后先执行这一步，再执行 Compile。
- `Game_Compile.ini`：把 PO/archive 编译为运行时使用的 `.locres`。
- `Config/DefaultGameUserSettings.ini`：保留无 Culture 覆盖时的策略说明；运行时由项目层解析器选择 Steam、系统或 `zh-Hans`，文件本身不预写 Culture。
- `Config/DefaultEditor.ini`：Localization Dashboard 维护的目标和工具设置可能写入这里。
- `Config/DefaultGame.ini`：负责打包阶段要携带哪些 Culture；应与 Game target 的 `CulturesToGenerate` 保持一致。
- `Content/Localization/Game`：保存项目 Game target 的 manifest、archive、PO、locmeta 和编译资源。

这些 `Game_*.ini` 文件保留了 Localization Dashboard 生成文件的说明。当前目标已按 Lyra 结构建立并验证；后续若在 Dashboard 中修改目标，应同步检查四个配置文件和 `Content/Localization/Game` 生成结果，避免手工翻译内容被覆盖。

### 5.2 资源与打包的关系

`GetLocalizedCultureNames(ELocalizationLoadFlags::Game)` 查询的是已经存在的 Game 本地化资源；`CulturesToStage` 查询的是打包要带哪些资源。两者必须同时满足：

1. `CulturesToGenerate` 生成了目标 Culture 的 `.locres`。
2. `CulturesToStage` 把对应 Culture 资源带进最终包。
3. Cook 后的产品中实际存在这些 Culture 的 Game 本地化目录。

只增加 `CulturesToStage` 而不生成翻译资源，不能让设置页面出现完整的语言列表；只生成资源而不 Stage，开发编辑器可能正常，打包版本仍会缺语言。

### 5.3 本次实施记录和路径边界

- `Game_Gather.ini` 的 `ManifestDependencies` 使用 `%LOCENGINEROOT%`，它只是 Unreal 本地化命令在编辑器/命令行 Gather 阶段解析的引擎目录宏，不是运行时路径，也不是提交到仓库的本机绝对路径。
- 运行时只依赖项目包内的 `Content/Localization/Game`；玩家不需要安装 Unreal Engine，游戏也不会读取开发机的引擎目录。
- 所有提交到仓库的本地化配置和资源路径均使用项目相对路径。开发机上命令行输出的绝对路径属于工具日志，不参与游戏运行和打包配置。
- 当前 Game target 已生成 11 个首发 Culture：`en`、`zh-Hans`、`ru`、`es`、`pt-BR`、`de`、`ja`、`fr`、`pl`、`ko`、`zh-Hant`；设置页另保留 Lyra 的 `System Default` 便利行。
- 本批 PO 已为 96 个新增 UMG 稳定键补齐 11 个 Culture 的翻译，并保留原有设置项、重启提示和服装 Tooltip 翻译；所有新增条目均已通过富文本标签和换行完整性检查。
- `UShootSessionCoordinatorSubsystem` 及相关 UI、Testing、Interaction、Pickups、System、Player 固定 `LOCTEXT`/`NSLOCTEXT` 源文案已统一为英文，并将 `HomeMap` 等内部 POC 名称改为面向玩家的 `Home Base`；明显测试/调试领取和武器测试区域提示已改为产品措辞，中文翻译使用“家园”。稳定 namespace/key 保持不变。
- `/Game/UI/Menu/WBP_GameMenu` 的 `ShowConfirmationYesNo` 节点曾把“退出游戏返回主菜单”和“你确定吗？”直接写成中文 `NSLOCTEXT`。该漏项位于 Widget Blueprint，不是 `UShootSessionCoordinatorSubsystem` 的 C++ 硬编码；现已统一为英文源文案 `Quit to Main Menu`、`Are you sure?`，并重新 Gather、Export、补译 11 个 PO、Import、Compile 生成 `Game.locres`。新 PIE 已验证弹框实际显示英文。
- 家园迁移后 `Config/DefaultGame.ini` 的 `SessionReturnMap` 已指向 `/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.HomeMap_Courtyard`；这是 Unreal 的项目资产路径，运行时和打包版只依赖包内资源，不依赖开发机磁盘路径。
- 截图中“已选择英语但退出副本弹窗仍是中文”的第一层原因是旧版本 `ShootMainMenuScreen` 的固定 `NSLOCTEXT` 源文案曾直接写成中文，且旧英文 PO 对应条目也保留了中文；修复后若不重启，旧 PIE/编辑器进程仍可能继续使用旧 C++ 模块和本地化缓存。冷编译并重启项目编辑器后，新的真实 PIE 已复现 `Leave Expedition`，确认框显示 `EXIT EXPEDITION`、`Leave the current expedition and return to the Home Base?...`；该现象不是另一会话把文案改回去了。
- `/Game/UI` 的 87 个 Widget Blueprint 已重新编译：0 个编译错误，2 个已有结构性警告（`W_SafeZoneEditor`、`W_HDRCalibrationEditor` 的 `Checkerboard` / `MID_Checkerboard` 成员引用解析警告）。本次同时修复了 `W_SafeZoneEditor`、`W_HDRCalibrationEditor`、`W_GammaEditor` 指向不存在的 Lyra 父类导致的编译错误，并刷新了 HDR 编辑器的过期事件节点引脚。
- 本地化命令的 Import、Compile 内部步骤均返回成功；Compile 生成了 11 个 Culture 的 `Game.locres`。命令行外层仍可能因本机未安装 LinuxArm64/VisionOS SDK 返回非零码，不能把该平台检查噪声误判为本地化步骤失败。
- Gather、Export、Import、Compile 的命令内部均返回成功；本机 AutomationTool 外层仍会报告缺少 LinuxArm64/VisionOS SDK 并返回非零码，这是平台 SDK 检查噪声，不是本地化命令失败。
- `WidgetService.capture_preview` 只能做设计时预览，设置页会依赖真实 `LyraLocalPlayer`；本次验收使用真实 PIE，避免把设计时预览断言误判成本地化故障。
- 真实 PIE 已从 HomeMap_Courtyard 进入家园游戏菜单，再打开 `/Game/UI/Settings/W_LyraSettingScreen`；语言行实际列出了 11 个首发 Culture 和额外的 `System Default`。在保存 `Culture=en`、重新生成本地化资源后，PIE 的家园菜单、退出主菜单确认框和 `Home Base` 文案均已显示英语；PIE 仍不能代替打包游戏首启验证。

## 6. 文本编写规则

后续每批 Gather 前，项目应继续遵守以下规则：

- C++ 固定用户可见文本使用 `LOCTEXT` 或 `NSLOCTEXT`，并使用稳定的 namespace/key。
- 带动态值的文字使用 `FText::Format`，动态值本身使用 `FText`，不要先拼成 `FString`。
- 数值、百分比和单位优先使用 `FText::AsNumber` 等本地化 API。
- 蓝图固定文案使用 Widget 的 Text 属性；蓝图中的 `Text` 资产属性应纳入 Gather。
- DataAsset 中的目录名称、描述和提示使用 `FText` 字段，不要在 C++ 构造函数中硬编码目录。
- 不要把源文本内容当成业务逻辑中的稳定 ID；namespace/key 一旦进入翻译资源，应尽量保持稳定。
- 只用于日志、调试和开发错误的文字先标注为非用户可见，不要无差别迁移。

示例：

```cpp
// 固定文案：进入本地化 Gather。
const FText Title = NSLOCTEXT("NewWorldOrder", "Revive_Title", "Revive");

// 动态文案：固定模板进入 Gather，数值保持为 FText。
const FText Message = FText::Format(
    NSLOCTEXT("NewWorldOrder", "Items_Count", "Items: {0}"),
    FText::AsNumber(ItemCount));
```

当前项目中使用 `FText::FromString(TEXT("..."))` 创建的固定用户文字，不会像 `LOCTEXT` 那样自动建立可翻译的稳定文本条目；本次审计范围内的固定中文 `FText` 已改为稳定的 `LOCTEXT/NSLOCTEXT`。后续新增固定用户文字仍应先确认可见性，再直接使用稳定的本地化宏。

## 7. 默认语言策略

### 7.1 已实施策略

首启逻辑采用：

```text
读取 GGameUserSettingsIni 的 [Internationalization] Culture
  -> 有有效 Culture：使用已保存语言
  -> 没有 Culture：匹配 Steam 应用语言
     -> 未匹配：匹配系统语言
     -> 未匹配：回退 zh-Hans
```

这样可以满足“首启无法识别时默认简体中文”，同时优先尊重 Steam/系统语言并保持用户后续选择的语言。Unreal 编辑器中的 PIE 运行在编辑器进程内，会优先沿用编辑器 Culture；因此 PIE 当前显示的语言不能证明打包游戏首启语言，首启策略应在非编辑器游戏进程或清洁用户配置中验证。完整重启提示沿用 Lyra，避免只刷新当前 Widget 后仍有部分已经创建的文本没有更新。

### 7.2 不要混淆三个概念

| 概念 | 作用 | 本项目建议 |
| --- | --- | --- |
| 首启默认语言 | 没有用户覆盖时的自动选择和回退 | Steam/系统匹配首发 Culture；都未匹配时 `zh-Hans` |
| `System Default` | 清除玩家覆盖并重新执行自动识别的选项 | 当前作为第 12 个便利选项保留，不计入首发 11 语种 |
| `NativeCulture` | 本地化源文本和未翻译回退的基准语言 | `en`；它不改变首启自动选择 |

Lyra 的 `ResetToDefaultCulture()` 语义是回到系统默认 Culture。若 NewWorldOrder 要求“重置语言 = 简体中文”，需要在项目层明确改变这个语义，不能只修改设置项显示名。

## 8. 实施阶段和后续清理

语言系统基础闭环已经落地，现有 C++ GameSettings/CommonUI 链路保持不变。本次增加了项目层 Steam、系统语言和简体中文回退解析，并已通过冷编译和 Live Coding 编译；本地化命令和 Widget 资产通过编辑器验证。`/Game/UI` 固定用户可见文案以及本次 `Private/UI`、`Private/Online`、`Private/Testing`、`Private/Interaction`、`Private/Pickups`、`Private/System`、`Private/Player` 范围内的固定 C++ 文案已完成首轮整理，明显测试/POC 玩家措辞已清理，且新 PIE 已验证英语退出副本确认框；剩余边界是动态/设计时样例文本、字体与布局，以及真实游戏进程中的切换持久化验收。

### 阶段一：创建本地化目标

1. 已按 Lyra 结构建立 `Game` target 配置文件。
2. 保留 `NativeCulture=en` 作为源文本和未翻译回退基准；首启语言由项目运行时按 Steam、系统、`zh-Hans` 顺序解析。
3. 已配置 11 个 `CulturesToGenerate`：`zh-Hans`、`en`、`ru`、`es`、`pt-BR`、`de`、`ja`、`fr`、`pl`、`ko`、`zh-Hant`。
4. 已 Gather C++、Config、蓝图和 DataAsset 中可被目标收集的 `FText`。

### 阶段二：翻译和编译

1. 已导出 PO，并为设置项核心文案和首轮 UMG 文案补齐简中、英语及其余 9 种语言。
2. 已通过 `Game_Import.ini` 导回 PO，再通过 `Game_Compile.ini` 生成各 Culture 的 `.locres`。
3. 已检查 `Game.manifest` 包含语言设置自身的 `LanguageSetting_Name`、`LanguageSetting_Description`、`SystemDefaultLanguage` 和重启提示等条目。

### 阶段三：项目层适配

1. 已将 `DefaultGame.ini` 的 `CulturesToStage` 扩展到 11 个 Culture。
2. 已由 `UShootGameInstance` 和 `ULyraSettingsShared` 为无用户覆盖的非编辑器游戏提供 Steam、系统、`zh-Hans` 自动识别；`DefaultGameUserSettings.ini` 不预写 Culture。
3. 当前保留 Lyra 的 `System Default` 可见行，作为不计入首发集合的第 12 个便利选项。
4. 当前语言列表继续由已生成的 Game locres 驱动，不在 C++ 构造函数中写 11 条硬编码数据。

### 阶段四：字体和运行验证

1. 已在真实 PIE 中打开游戏菜单和设置页，确认语言行可见并列出 11 个首发语言及 System Default。
2. `/Game/UI` 资产已重新编译，0 个错误；新 PIE 已验证英语退出副本确认框，真实游戏进程仍需逐一切换 11 个 Culture，重点检查字体、布局和重启行为。
3. 待验证应用、取消、重置、退出后重新启动和 GameUserSettings 配置持久化。
4. 待用全新用户配置验证首次启动确实为简体中文；编辑器 PIE 不作为此项的唯一证据。
5. 特别验证 `zh-Hant` 在 UE 5.8.1 中的 Cook、运行时加载和字体显示。

### 阶段五：UMG 文案清理

1. 已以页面为单位扫描 `/Game/UI` 下 87 个 Widget Blueprint 的 `Text`、`ButtonText`、`ExperienceDisplayName` 和 `PreviewText` 属性。
2. 已将 104 处固定用户可见文案迁移为稳定 `NSLOCTEXT` 键，保留控件布局和视觉配置在 Widget Blueprint。
3. 已重新 Gather、Export、补齐 11 个 Culture 的 PO、Import、Compile，并完成 87 个 Widget Blueprint 编译审计。
4. 设计时样例、动态占位符、纯数字/符号、调试文字和产品品牌字样不纳入本批固定文案迁移；这些边界已记录，避免把运行时数据误当成翻译源文本。

### 阶段六：持久化验收清单

以下步骤留给用户在关闭其他编辑器占用后执行。本次已完成资源和资产侧准备，但没有把 PIE 中的编辑器 Culture 现象当作持久化验收证据。

1. 使用全新用户配置，或启动 Standalone/打包游戏并确认没有旧的 `GameUserSettings.ini` 覆盖默认值。
2. 首次进入 FrontEnd 和设置页，确认默认显示为简体中文 `zh-Hans`。
3. 在设置页选择英语，点击 Apply，接受语言变更需要完整重启的提示；重启后确认菜单、设置和已纳入范围的 UMG 文案切换为英语。
4. 关闭游戏并重新启动，确认英语仍保持；检查用户配置 `[Internationalization]` 下存在 `Culture=en`。
5. 至少再重复一次俄语，以及一次 CJK 语言（建议 `ja` 或 `zh-Hant`），检查重启后仍保持并确认字体、换行、按钮宽度正常。
6. 选择 `System Default`，Apply 并重启；确认 `GameUserSettings.ini` 中的 `Culture` 键被移除，正式运行时按 Steam、系统、`zh-Hans` 顺序重新识别。
7. 逐项验证 Cancel、Reset 和离开设置页：Cancel 不应改变已应用语言；Reset/System Default 的语义是清除覆盖并重新执行项目自动识别策略。
8. PIE 可能沿用编辑器自身 Culture，且当前进程语言状态会影响已创建 Widget；因此 PIE 只能辅助检查点击链路，不能替代全新配置的真实游戏/打包验收。
9. 玩家电脑不需要安装 Unreal Engine。`%LOCENGINEROOT%` 只在开发机的 Gather/Compile 命令阶段解析，运行时和打包版只加载项目包内 `Content/Localization/Game` 的资源。

## 9. 风险与验收标准

### 风险

- 没有 Game `.locres` 时，语言设置项无法按预期枚举项目语言。
- 只添加 Stage 配置而没有 Compile 资源时，打包版仍会缺语言。
- `FText::FromString` 固定文本不会自动进入同等的 C++ 本地化 Gather 流程；本批固定 C++ 文案已改为稳定 `LOCTEXT`/`NSLOCTEXT` 源文本。
- `zh-Hant` 需要真实验证，不能仅凭 Culture 字符串通过就认为打包链路完整。
- CJK 与西里尔字母可能出现字体缺字、回退不一致或 UI 宽度变化，需要逐语言截图检查。
- 当前 Lyra 设置项使用资源发现顺序，未承诺需求表中的显示顺序。
- Culture 是进程级状态；本地分屏不能按玩家分别切换不同语言。

### 验收标准

- 全新配置启动后，FrontEnd/设置页面默认显示简体中文。
- 设置页面显示首发 11 种语言，顺序符合产品决定；若保留系统默认，页面明确显示为额外选项。
- 每种语言的 Game `.locres` 都在开发和打包版本中可加载。
- 切换后 Apply 能保存，Cancel 不会改变已应用语言，重启后保持选择。
- UI、弹窗、交互提示和 HUD 中已纳入范围的固定文字都能找到对应翻译或有明确的源语言回退。
- 11 种语言均无明显缺字、溢出、重叠和 CommonUI 焦点异常。

本次已完成的证据：11 个 Culture 均有 UMG 与本批 C++ 固定文案的非空 PO 翻译及 `Game.locres`；C++ 固定用户文案源文本已统一为英文，明显测试/POC 玩家措辞已清理；冷编译后新 PIE 已验证英语退出副本确认框和 `Home Base` 文案；`/Game/UI` 87 个 Widget Blueprint 编译结果为 0 错误、2 个警告。仍待用户完成的证据：真实游戏进程中的默认语言、Apply/Cancel/Reset、完整重启和 `GameUserSettings.ini` 持久化检查。

## 10. 证据和源码索引

### NewWorldOrder

| 文件 | 调查重点 |
| --- | --- |
| `Source/NewWorldOrder/Private/Settings/CustomSettings/LyraSettingValueDiscrete_Language.cpp` | 语言选项发现、显示名称、pending Culture、Apply 警告、重置和 Culture 回退 |
| `Source/NewWorldOrder/Public/Settings/CustomSettings/LyraSettingValueDiscrete_Language.h` | 语言设置类声明和可用 Culture 状态 |
| `Source/NewWorldOrder/Private/Settings/LyraSettingsShared.cpp` | `SetCurrentCulture`、`GGameUserSettingsIni`、pending/reset 状态和 CultureChanged 委托 |
| `Source/NewWorldOrder/Public/Settings/LyraSettingsShared.h` | SharedSettings 公开的 Culture 接口 |
| `Source/NewWorldOrder/Private/Settings/LyraGameSettingRegistry_Gameplay.cpp` | Gameplay 集合中注册 Language 设置 |
| `Source/NewWorldOrder/Private/Settings/LyraGameSettingRegistry.cpp` | 设置注册表初始化和 SaveChanges 链路 |
| `Source/NewWorldOrder/Private/UI/Settings/LyraSettingScreen.cpp` | 设置页面的 Apply、Cancel、Reset、DeactivateWidget 和 LocalPlayer 入口 |
| `Config/DefaultGame.ini` | 当前 `InternationalizationPreset` 和 `CulturesToStage` |
| `Docs/Tasks/SessionUI/Requirements_需求.md` | 设置页面需求和 `/Game/UI/Settings/W_LyraSettingScreen` 入口 |
| `Docs/Tasks/SessionUI/DesignSpec_设计规格.md` | 当前设置系统采用 Lyra GameSettings/GameSubtitles 迁移方案 |
| `Docs/DevelopmentNotes/MCP_踩坑记录.md` | UE 5.8.1、VibeUE 和资产操作约束 |

### Lyra 与 Unreal Engine 对照

| 参考路径 | 调查重点 |
| --- | --- |
| `Source/LyraGame/Settings/CustomSettings/LyraSettingValueDiscrete_Language.cpp` | Lyra 语言设置项原实现 |
| `Source/LyraGame/Settings/LyraSettingsShared.cpp` | Lyra Culture 应用和 GameUserSettings 持久化 |
| `Source/LyraGame/Settings/LyraGameSettingRegistry_Gameplay.cpp` | Lyra 语言设置注册 |
| `Config/Localization/Game_Gather.ini` | Lyra Game target 的 Gather 范围和步骤 |
| `Config/Localization/Game_Export.ini` | Lyra PO 导出配置 |
| `Config/Localization/Game_Compile.ini` | Lyra `.locres` 编译配置 |
| `Content/Localization/Game` | Lyra manifest、archive、PO 和 locres 资源组织 |
| `Engine/Source/Runtime/Core/Public/Internationalization/TextLocalizationManager.h` | `GetLocalizedCultureNames(Game)` 的资源发现语义 |

## 11. 调查后的实施判断

语言切换不需要另起一套 UI 系统，也不需要修改 Lyra 上游源码。当前实现保留项目现有 GameSettings/CommonUI 链路，补齐了 Game 本地化目标、11 种资源、Steam/系统/简中首启策略和 `/Game/UI` 固定用户文案；本批 C++ UI/Online/Testing/Interaction/Pickups/System/Player 固定文案也已统一为英文源文本并清理明显测试/POC 玩家措辞，设置页已经能从 Game locres 发现 11 种首发语言，87 个 Widget Blueprint 也已完成编译审计；`WBP_GameMenu` 的退出主菜单确认框漏项也已修正并在 PIE 验证。

闭环的运行时边界已经明确：本地化配置中的 `%LOCENGINEROOT%` 只服务于编辑器命令阶段，打包后的游戏只加载项目包内的 `Content/Localization/Game`。后续按阶段六完成真实游戏进程中的每种语言重启、字体、布局和持久化验收。
