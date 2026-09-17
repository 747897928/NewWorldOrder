# Codex 项目架构对账

下面的 Gameplay 总规格由 GPT-5.6 Sol 协助用户整理。我负责根据用户的玩法目标提供外部 Gameplay / 架构建议，你掌握当前仓库、历史修改和项目上下文。

请先阅读：

`Gameplay_Product_Gameplay_Architecture_FullSpec_v0.3.md`

本轮先做项目现实状态对账，输出基于真实类、资产和代码路径的结论。

请覆盖以下内容：

1. 当前 Experience 系统实际结构，以及与 Lyra 原版相比保留、裁剪、改写的部分。
2. GameMode、GameState、PlayerState、PlayerController、Pawn、PawnData、ASC、AttributeSet、Health、Damage、Death、Respawn、Team、GameFeature、UI、Inventory、Equipment、Weapon 的实际职责和关键类名。
3. 当前 Experience 如何决定 Pawn、Ability、Attribute、UI 和玩法模块。
4. 当前代码中已经存在的模式耦合、硬编码和未来扩展风险。
5. Listen Server 下 Host 与 Remote Client 的 Authority / Replication 风险。
6. 现有 PvE Zombie 的完整实现链路：Character、AIController、BehaviorTree、Blackboard、AnimBP / AnimInstance、Locomotion、Attack、HitReact、Death、Health、ASC、Team、Spawn、Replication。
7. 现有 PvE Zombie 哪些属于可复用基础，哪些属于 POC。
8. 用总规格中的几个实际场景检查当前架构：
   - HomeMap 隐藏 Combat HUD。
   - Human -> Zombie。
   - Challenge Limited Lives / Respawn。
   - 大灾变 Match Currency / Attribute Upgrade。
   - 原创 PvE 四技能槽、Round 三选一、技能三级、Match Gold、普通商店、神秘商人。
9. 当前 Experience 移植继续推进前，最值得先修正的架构问题。
10. 可以保留到未来再做的内容。

GPT-5.6 Sol 没有完整仓库上下文。总规格中的技术判断如果与现有项目不匹配，请直接给出更准确的实现关系、原因和代码证据。产品玩法目标仍以用户明确需求为准。

本轮输出项目审计和建议，不执行大规模重构。
