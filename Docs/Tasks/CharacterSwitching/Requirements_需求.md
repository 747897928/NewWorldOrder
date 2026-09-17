# 角色切换需求
版本：1.2
最后更新：2026-03-20

## 功能需求
1. 支持在同一局/同一存档内无缝切换男女主。
2. 共享数据：等级、经验、剧情进度、账号资源（ResourceInventory）。
3. 独立数据：外观（Mutable）、技能/GA 列表、属性（AttributeSet 初始值）、装备与库存（InventoryManager 持有的 ItemInstances）、QuickBar 绑定。
4. 切换流程必须原子：任一步骤失败需回滚到切换前状态。
5. 网络：服务器权威执行切换；客户端只接收复制/GameplayCue/表现更新。
6. 性能：切换时不销毁 Controller/PlayerState/ASC/InventoryManager/ResourceInventory/CombatComponent。

## 与衣柜换装的关系（2026-06-13 补充）
- 角色切换和衣柜换装是两个前端功能，但共享同一份角色外观数据。
- 角色切换负责改变当前操控主角；衣柜负责编辑当前主角的服装搭配。
- 切换角色时必须保存当前主角外观，并加载目标主角上次保存的外观。
- 衣柜打开时展示的“当前角色”必须来自 PlayerState 当前性别，不能由 Widget 自己维护一份不受控性别状态。
- 当前阶段衣柜只编辑当前操控角色，不做男主女主同屏换装编辑。
- 服装所有权由库存系统持久化；角色切换只保存每个角色当前装备了哪些外观标签或 Descriptor。
- 当前 POC 蓝图 `W_Cloth` 与 `W_Cloth_Item` 只验证女角色硬编码换装可行，后续不能继续把它们当作最终数据源。
- 详细衣柜需求见 `Docs/Tasks/WardrobeSystem/Requirements_需求.md`。

## 目标交互形态（按《刺客信条：枭雄 / 影》理解后的明确版）
- 主入口：Hub / 安全屋 / 家园内，当前操控角色靠近“另一位主角”时出现交互提示，长按交互键后切换角色。
- 次入口：按 `ESC` 打开主菜单或 CommonUI 菜单，在满足切换条件时提供“切换角色”入口。
- 两种入口共用同一套后端切换逻辑，只是前端触发方式不同。
- 世界内入口不再弹二次确认框，长按本身就是确认动作。
- 菜单入口也推荐做成长按，不再额外叠一层确认弹窗。
- 非 Hub / 非安全屋 / 副本战斗中，不显示或不允许切换入口。
- Hub 内应存在另一位主角的可见实体或可交互表现，不能把最终体验长期停留在抽象“切换站”上。

## 双主角玩法理解（当前已定）
- 菜单入口更接近《刺客信条：影》的“当前操控角色切换”：
  - 本质是单选
  - 当前只能操控男主或女主其中一个
  - 这条入口更像角色选择或操控权切换，不属于世界交互
- 世界入口更接近《刺客信条：枭雄》的安全屋 / 火车内切换：
  - 当前主角靠近另一位主角 NPC
  - 触发一个上下文交互
  - 切换当前操控权
- 这两条入口前端不同，但后端必须共用。

## 未来扩展边界（当前只记录，不进入本轮验收）
- 另一位主角 NPC 后续不应被设计成“只会切换角色的按钮”。
- 未来规划中，这个 NPC 还可能提供其他交互：
  - 对话
  - 邀请跳舞
  - 送礼
  - 约会
  - 队友互动
- 因此中长期设计方向应是：
  - “另一位主角 NPC”本质上属于同伴交互对象
  - “切换角色”只是该对象的一种交互选项
- 但当前阶段按方案一执行：
  - 先完成双主角切换主线
  - 不把亲密度、跳舞、送礼等同伴交互需求混入本轮功能实现

## 参考依据（外部资料）
- 《刺客信条：枭雄》双主角官方资料：
  - Ubisoft News:
    - https://news.ubisoft.com/en-au/article/5GAu1o4oes5nTDh6BskzW5/assassins-creed-syndicate-build-an-underworld-empire-in-victorian-london-with-ubisoft
  - Ubisoft:
    - https://www.ubisoft.com/en-us/game/assassins-creed/news/5wlBeqYoDHhzrh61xMNj7R/assassins-creed-syndicate-play-now-at-60-fps-on-xbox-series-xs-ps5-and-ps5-pro
- 《刺客信条：影》双主角官方资料：
  - Ubisoft Québec:
    - https://quebec.ubisoft.com/en/assassins-creed-shadows-launches-november-15-features-dual-protagonists-in-feudal-japan/
  - Ubisoft News:
    - https://news.ubisoft.com/en-gb/article/1hges9IWLOmNAFpiavYmKl/the-evolution-of-assassins-creeds-stealth
- 《影》的菜单切换与受限场景体验，当前只作为玩法观察，不作为权威规则来源：
  - Game8:
    - https://game8.co/games/Assassins-Creed-Shadows/archives/504558
  - Gamepressure:
    - https://www.gamepressure.com/assassins-creed-shadows/naoe-and-yasuke/zf117e1

## 与《游戏设计完整文档 v7.0》对齐
- 切换限制：仅允许 Hub 场景切换；副本战斗中禁止切换。
- 预设差异：男女主背包/QuickBar 预设不同（示例：男主 AK47+手枪1+近战1；女主 狙击枪+手枪2+近战2），切换需加载各自预设并保存回快照。
- 槽位规则：按设计文档的武器槽位（主武器/副武器/近战），高等级可解锁第二主武器槽（Lv20）。G 键丢弃、数字键切换需保持一致。
- RL/GL/手榴弹：爆炸武器类别存在（RL/GL/投掷物），切换后库存/QuickBar/弹药 StatTags 必须保持各自独立。
- Hub 设施：武器仓库/商店/角色切换站 UI 预期仍可用，切换不应破坏装备/存储逻辑。
- 主动/被动/等级：
  - 每角色 7 个主动技能（Q/E/F 等）+ 4 通用被动 + 2 角色专属被动；Lv5/10/15/25 依次解锁被动槽和可选技能槽。
  - 属性差异：男主坦克（力量/体力倾向，HP +15%，移速 -5%），女主机动辅助（敏捷/感知倾向，HP -10%，移速 +15%，射击惩罚降低）；切换需加载各自属性/被动/技能配置。
  - 被动影响武器：敏捷/被动可影响换弹速度、射击惩罚；切换时需刷新 ASC 上的被动效果并同步至武器/GA。

## 非目标 / 边界
- 不新增第三角色；仅男女主双档。
- 不支持跨账号共享（同一 PlayerState 范围内）。
- 不做旧存档兼容：必要时可重置或迁移存档，但需在 STATUS 说明。
- 不引入新的 Actor 依赖；外部接口只暴露 UObject（WeaponInstance/ItemInstance），Actor 仅内部视觉壳。
- 当前 `AShootCharacterSwitchStation` 可作为开发期或占位实现，但不是最终体验定义本身。
- 世界内确认弹窗不再是目标体验，只保留切换结果提示与长按进度表现。

## 约束
- 遵守 InventorySystem 架构：ItemDefinition+Fragments（WeaponBasic/Ranged/Projectile/Equippable），ItemInstance+StatTags，WeaponInstance 驱动 GA/HUD；AShootWeaponActor 仅视觉。
- GAS：Simulated Client 不执行 GameplayAbility；服务端/本地控制端处理状态变更；表现用 GameplayCue/复制。
- 交互主逻辑尽量落在 C++：
  - `UShootGA_Interact` 是权威实现
  - 蓝图只保留最少的资产配置和 UI 表现
  - 旧蓝图 `GA_Interact` 如需继续存在，只允许作为继承 `UShootGA_Interact` 的薄包装
- 代码复杂度：函数行数尽量≤80，嵌套≤3，保持单一职责；公共头文件使用前置声明减少依赖。

## 测试要求（回归）
- 单机：切换前后外观/技能/属性/装备/库存保持各自独立，Shared 数据保持不变；武器开火/换弹/拾取仍正常。
- Listen Server：切换在服务器生效，客户端表现同步；拾取/丢弃/QuickBar/弹药/投射物无回归。
- 存档：保存男主→切换女主→保存→重载存档→验证两份独立快照与共享数据。
- 交互：长按进度条会推进；松开会回落；读满才触发切换。
