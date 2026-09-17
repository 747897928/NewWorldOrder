# UMGLayout FillSpacer 布局笔记

日期：2026-07-09
状态：[可用]

## 结论

- UMG 内部比例布局优先使用 `HorizontalBox`、`VerticalBox` 子项 Slot 的 `Size = Fill` 与 `Fill Value`。
- 空白区域使用 `Spacer` 承担占位。
- `CanvasPanel`、`Overlay` 适合做顶层锚点、覆盖层和大区域分组。
- 衣柜这类角色预览页面适合用横向比例布局：左侧 UI 区、中间或右侧角色预览区、底部预览控制区。
- 角色预览长期方向：人物放在右侧内容区，服装列表和页签按比例让出预览空间。

## 示例

```text
Root Overlay
├─ Background
├─ Safe Zone
│  └─ Horizontal Box
│     ├─ Left Menu Panel       Fill = 35
│     └─ Right Character Panel Fill = 65
└─ Foreground Effects
```

```text
Right Character Panel
└─ Overlay
   ├─ Character Preview
   ├─ Bottom Center Controls
   └─ Costume Background Layer
```

## W_Cloth 记录

- `/Game/UI/Mutable/W_Cloth` 当前根 Overlay 的第一层是 `CharacterPreviewPanel`，角色和影棚作为页面底层。
- `Overlay_0` 内使用 `MainContentHBox`：`MainBorder` 为 Fill 58，`PreviewRightSpacer` 为 Fill 42。
- `MainBorder` 已迁出 Overlay 绝对 padding；内容面板通过 Fill 权重给右侧角色预览区让位。
- `PreviewControlsHBox` 仍在 `CharacterPreviewOverlay` 内，当前靠右下方放置，后续可按最终视觉稿迁到右侧预览控制容器。
- `ToggleUIShortcutHint` 放在 `WardrobeActionPanel` 内，普通状态提示 `H / Y Hide UI`，隐藏 UI 时随动作面板一起收起。
- 预览按钮和右下角动作按钮自身必须保持 `Visible`。容器可以 `Self Hit Test Invisible`，按钮本身不可以，否则 Designer 里看得见但运行时点不到。
- 固定按钮的文字、图标、尺寸和位置由 `W_Cloth` 蓝图维护；C++ 只保留点击进入业务逻辑的绑定。

## 来源

- Epic `UHorizontalBox` 文档：https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/UHorizontalBox
- Epic `UHorizontalBoxSlot` 文档：https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UHorizontalBoxSlot?lang=en-US
- Epic `USpacer` 文档：https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/USpacer?lang=en-US
