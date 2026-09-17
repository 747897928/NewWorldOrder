# Lyra Widget Style 换装卡片参考

日期：2026-07-08

## 参考资产

- `/Game/UI/Weapon/WBP_ActionTouchButton`
  - EventGraph 使用 `Get Dynamic Material` 取得 Image 的动态材质，再写 `Animate_FromEmpty`、`Animate_Cooldown`、`Animate_ActiveToInactive` 等标量参数。
  - 动画包括 `CooldownInactiveToActive`、`CooldownActiveToInactive`、`OnClickedTouchInput`。
  - 适合参考材质驱动的触摸按钮反馈，不适合把业务状态写进 Widget 图表。

- `/Game/UI/Weapon/WBP_WeaponSlot`
  - EventGraph 很薄，只有默认空事件。
  - 状态反馈主要靠 `InactiveToActive`、`ActiveToInactive`、`ToEmpty` 三个 Widget Animation。
  - `ActiveToInactive` 是衣柜卡片选中态退场的优先参考：把视觉变化留在蓝图动画里，C++ 只切状态。

## W_Cloth_Item 当前结论

- `/Game/UI/Mutable/W_Cloth_Item` 继承 `UShootWardrobeItemWidgetBase`，数据和交互主线由 C++ 的 `SetItemData`、点击、hover/focus 转发驱动。
- 旧 EventGraph 中的 `Struct_Cloth_Attribute`、直接调用 Mutable、Cast 角色、`Server Set Selected Option and Update Mesh` 都已清空。
- 旧函数图 `SetClothIcon`、`SetStructClothAttribute`、`SelectClothItem` 已删除。
- 当前保留 `HoverEffect` 动画和绑定控件：`BackingBorder`、`ItemIcon`、`RarityText`、`EquippedCheckText`、`ItemNameText`、`StatusText`。
- `Frame` 和 `SelectBorder` 的旧直角发光边框已从资产和 C++ 绑定中删除，不用 `Collapsed` 保留废弃视觉层。`BackingBorder` 使用 Lyra 快捷栏同源的圆角 brush，是卡片底板和 selected/focus 视觉的唯一承载层。

## 后续规则

- 换装卡片的视觉层可以学习 Lyra 武器槽：底板、阴影、选中框、材质参数和 Widget Animation 放在 UMG 蓝图。
- 不要在 `W_Cloth_Item` EventGraph 里写库存、Mutable、角色 Cast、装备 RPC 或旧 DataTable 逻辑。
- 若需要更好看的选中态，优先在 `BackingBorder` 上新增 `InactiveToActive` / `ActiveToInactive` 这类动画或材质参数；C++ 只暴露清晰的选中/未选中状态入口。
