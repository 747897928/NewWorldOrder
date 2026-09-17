# ADR-0001: UI 蓝图与 C++ 职责边界（含条目交互语义委托模式）

## 元数据

```yaml
adr_id: ADR-0001
title: UI 蓝图与 C++ 职责边界（含条目交互语义委托模式）
date: 2026-08-13
status: Active
author: AI + 用户
related_docs:
  - AGENTS.md
  - Docs/Engineering/Notes/UI/CommonUI_架构与用法.md
  - Docs/Tasks/WardrobeSystem/Implementation_实现指南.md
  - Docs/DevelopmentNotes/AdventurersInventoryKit_Tooltip_蓝图实现调研.md
```

## 决策

控件蓝图与 C++ 按四条通道分层：蓝图管表现装配，C++ 管业务与状态，ViewModel 管可绑定数据快照，GameplayMessage 管跨系统通知。动态条目与页面的交互事件一律通过条目基类的语义委托汇入页面协调层；条目蓝图不持有页面引用、不直接调服务器。

## 背景

- 旧 `UShootWardrobeWidgetBase` 把数据查询、分类、动态控件、焦点、Tooltip 定位、预览相机塞进约 1659 行 C++ 与 33 个 BindWidget，蓝图资产被 C++ 父类绑架。
- MVVM 迁移后 Tooltip 悬停功能静默丢失：悬停入口被规划为“蓝图发事件”，但没有任何一层实现，C++ 也没有兜底；多轮协作下无人能在短时间内看出断点在哪。
- 项目需要其他 AI 和同事能快速看懂代码：事件入口、数据源、更新路径必须成对出现、可 grep、可复读。

## 备选方案

### 方案A：逻辑尽量放蓝图 EventGraph

优点：表现相关改动快，无需编译。

缺点：图无法 grep 审计；多 AI 协作容易各写一套；找不到数据源与断点（本次悬停回归的直接教训）。

### 方案B：逻辑尽量塞进大型 C++ Widget 基类

优点：可编译、可 grep。

缺点：BindWidget 强耦合控件树；大类难以维护；蓝图资产迁移会被 C++ 父类绑架。

### 方案C（选定）：四条通道分层 + 语义委托

优点：交互入口唯一（条目基类委托）；数据源唯一（ViewModel 属性）；表现自由（蓝图绑定/动画）；新接手者可沿“事件 -> 委托 -> 协调层 -> ViewModel -> 绑定”一条线重建上下文。

缺点：小改动也要动 C++；需要团队遵守纪律。

## 后果

- 后续 UI 开发按下方判定清单执行。
- 条目级交互新增（悬停/焦点/右键）优先扩展 `UShootObjectEntryButtonBase` 的语义委托，不新增旁路。
- 同类“功能静默丢失”排查顺序：数据源是否唯一 -> 谁写数据源 -> 事件入口在哪一层 -> 绑定是否存在。

## 判定清单（什么时候放哪）

- 布局、尺寸、颜色、样式、动画、固定文案、图标 -> 控件蓝图 Designer / EventGraph。
- 固定按钮 OnClicked -> 页面 EventGraph，调用语义明确的 BlueprintCallable。
- 可绑定的标量/数组 UI 快照（分类、条目、选中项、拥有状态）-> `UMVVMViewModelBase` + FieldNotify。
- 库存、服务器权威、Mutable、SaveGame、网络复制、输入状态 -> C++（PlayerState/Controller/ViewModel 服务侧）。
- 动态条目与页面的交互（点击/悬停/焦点）-> 条目基类语义委托 -> 页面协调层 -> ViewModel；条目不存页面引用。
- 跨系统状态变更通知 -> `UGameplayMessageSubsystem`。
- 跨设备输入 -> CommonUI 设备无关语义（如 `NativeOnHovered`）+ Enhanced Input IA/IMC；禁止 `if(Key==)` / `IsGamepad()` 分支。
- 蓝图图表复杂度本身就是信号：当 EventGraph / Function 出现“蜘蛛网”（节点过多、连线交叉、嵌套分支过深、同一段逻辑复制多份）时，说明该段业务已超过蓝图可读上限，应抽成语义明确的 C++ 方法（BlueprintCallable / BlueprintNativeEvent），蓝图只保留“事件 -> 调用 -> 视觉装配”。反向同样成立：简单直观的数据装配不要为了减少连线强行搬进 C++。规则不是死的，准绳是“下一个接手者能否在几分钟内看懂并安全修改”。
- 禁止：C++ 硬编码颜色尺寸文本、FindWidget 按名找控件、条目蓝图直连 RPC、用 Collapsed/Hidden 伪装废弃路径。
