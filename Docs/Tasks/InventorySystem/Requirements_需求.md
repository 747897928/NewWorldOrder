# 功能需求汇总

## 1. 背包结构
- 角色存在“武器槽”（男女预设不同）与“材料栏/杂项”两大区域，互不影响容量。
- 材料栏需要自动拾取、自动堆叠、无重量限制，Tab → 背包 → 材料标签展示所有堆栈。
- 已拾取物品不可在章节重置后再次生成，须支持持久化（存档或服务器状态）。

## 2. 物品分类与约束
- 材料类（堆叠资源）：军用合金（≤999）、服装碎片（≤9999）、金币（≤99999）。
- 稀有道具（不可堆叠）：势力徽章 5 种、传说武器设计图 5 张。
- 消耗品/材料需要 UI 提示（右上角“+2 军用合金”）和稀有度光柱。
- 未来还需扩展到消耗品、材料、制作配方、任务物品、组件等（GameDesign 7.0 §7.3/7.4、章节奖励列表）。

## 3. 制作与消耗
- Hub 仓库 NPC “军械师·老赵”提供武器/服装制作 UI，左侧卡片、右侧详情与条件。
- 传说武器统一配方：对应势力徽章×1 + 设计图×1 + 军用合金×50 + 金币×5000。
- 服装制作分普通/稀有/传说：普通=碎片 100；稀有=碎片 200 + 合金 10 + 金币 2000 + 徽章。
- 制作按钮需实时读取背包数据，条件满足显示绿色勾并可点击，否则灰色禁止。
- 完成制作需：扣减材料、触发 3 秒锻造动画、将武器实例入库、更新成就/NPC 对话。

## 4. 系统交互
- 掉落源：敌人、Boss、副本评级、随机事件、宝箱、隐藏房间；评级倍率影响掉落数量。
- 背包需记录拾取历史防止刷材料，且要在多人同步场景下保持权威。
- 传说武器制作完成后要驱动任务系统/成就系统，制作条件 UI 需要实时联动背包状态。
- 武器、消耗品、材料都要成为 GAS Ability Cost/TargetData 的数据源，以支持预测与回滚。

## 5. 技术约束
- 权威扣减必须在服务器，客户端仅预测 UI（沿用当前 PredictionKey 思路）。
- 背包/材料栈需要高效复制（FastArray or TagStack）并提供 `PreReplicatedRemove/PostReplicatedAdd` 钩子以更新 UI。
- 需要可扩展的 ItemDefinition/Fragment 体系以承载额外属性（制作配方、装备授予的能力等）。
- 新系统必须兼容现有 `UCombatComponent` 和 `AShootWeaponActor`（短期内 Quickbar 仍然有效）。

## 6. 生命周期与存档（2025-11-15）
- PlayerState 必须同时挂载 `UResourceInventoryComponent`（堆叠资源仓库）与 `UShootInventoryManagerComponent`（有身份背包），职责严禁混用。
- Inventory ItemInstance 需支持 `EShootItemLifetime`（Persistent / RuntimeOnly），并由 InventoryManager 提供 `AddPersistentItem` 与 `AddRuntimeItem` 接口；副本内拾取的武器一律走 RuntimeOnly 流程。
- SaveGame 与 QuickBar 配置仅包含 `Persistent` 实例；退出副本或返回 Hub 时，RuntimeOnly 物品全部清空，不写存档、不修改账号 QuickBar。
- `UResourceInventoryComponent` 永远保存 Persistent 资源（材料/徽章/设计图/金币等），战斗内一次性增益通过 Ability/拾取即时处理，禁止出现 RuntimeOnly 资源。
- QuickBar、EquipmentManager 以及武器/装备 GA 只能读取 InventoryManager 提供的实例引用，确保 Hub 配置与副本内拾取在同一条逻辑线上。

## 7. 服装与衣柜需求（2026-06-13）
- 服装属于 Persistent 有身份物品，必须进入 `UShootInventoryManagerComponent`，不能只存在于衣柜 Widget 或 Mutable 参数里。
- 服装碎片、军用合金、金币、势力徽章、设计图属于数量型资源，继续由 `UResourceInventoryComponent` 管理。
- Hub 仓库制作服装时，服务器从 ResourceInventory 扣减材料，再通过 InventoryManager 添加 Persistent 服装 ItemInstance。
- 衣柜 UI 只能把 InventoryManager 中已拥有的服装显示为“已拥有 / 可装备”，不能把静态表里的全部服装都当成玩家拥有。
- 衣柜可以展示可制作或未解锁服装，但必须区分“已拥有、可制作、未解锁、不可用于当前角色”。
- SaveGame 需要保存 Persistent 服装 ItemInstance，以及男女主各自当前装备的外观标签或 Descriptor。
- 服装所有权可以是账号共享池；男主与女主当前穿搭必须独立保存。
- 调试用测试物品提供者必须能发放服装、武器、资源三类奖励，并严格走 InventoryManager / ResourceInventory 正式入口，避免再出现硬编码 POC。
