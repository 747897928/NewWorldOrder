# Protagonist_TODO_主角技能收敛

- 目的：汇总当前工程内所有 `TODO(主角-...)`，并拆分为可执行子任务（SSOT）。
- 维护规则：
  - 关闭/实现任意 `TODO(主角-...)` 时，必须同步更新本表（删除或标记已完成，并补充变更点）。
  - 新增 `TODO(主角-...)` 时，必须补充到本表（包含：路径、函数上下文、问题描述、预期目标）。

---

## 子任务清单（可执行）

1. 阵营/敌我查询统一化
- 关联 TODO：`主角-TacticalScan-TeamFilter`
- 位置：`Source/NewWorldOrder/Private/AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_TacticalScan.cpp:96`
- 现状：新增 AbilitySystemLibrary::IsHostileActor，优先 TeamId，缺失时回退 Faction Tag；TacticalScan/MarkHunter 已接入。
- 目标：若未来新增更多阵营来源，补齐该入口即可。

2. RescueCloak 移速倍率可配置
- 关联 TODO：`主角-RescueCloak-MoveSpeedBonus`
- 位置：`ShootGA_Female_RescueCloak.cpp:25`
- 目标：将 MoveSpeedBonus 通过 SetByCaller 或专用 GE 生效，替换固定倍率 GE。（已实现）

3. RescueCloak 隐身表现与感知
- 关联 TODO：`主角-RescueCloak-CloakBehavior`
- 位置：`ShootGA_Female_RescueCloak.cpp:46`
- 目标：AI 感知与 UI 订阅 Status.Cloaked，完成隐身显示/感知逻辑。（已实现攻击破隐，感知/UI 仍缺）

4. RescueCloak 救援联动
- 关联 TODO：`主角-RescueCloak-Revive`
- 位置：`ShootGA_Female_RescueCloak.cpp:47`
- 目标：接入复活/交互系统，包含救援速度加成、救起后保护窗口、队友范围隐身。
- 现状：救起后治疗与保护已接入事件回调；救援速度倍率已注入交互事件并由救援交互能力读取；新增救援交互组件（需倒地状态启用）；队友范围隐身仍缺。

5. RapidCharge 击杀归属统一
- 关联 TODO：`主角-RapidCharge-KillAttribution`
- 位置：`ShootGA_Female_RapidCharge.cpp:85`
- 现状：RapidCharge/MarkHunter 已统一使用 `UShootAbilitySystemLibrary::IsKillAttributedToActor`。
- 目标：若后续接入独立 Combat/Score 服务，可在该统一入口替换实现。

6. MedicalStation 高级规则
- 关联 TODO：`主角-MedicalStation-Design`
- 位置：`ShootGA_Female_MedicalStation.cpp:46`
- 目标：实现治疗量缩放、死亡保护与额外交互。

7. 被动授予即激活机制
- 关联 TODO：`主角-PassiveAutoActivate`
- 位置：
  - `ShootGA_Passive_MedicalExpertise.cpp:19`
  - `ShootGA_Passive_SmartAssist.cpp:19`
  - `ShootGA_Passive_MarkHunter.cpp:22`
  - `ShootGA_Passive_ArmorEnhancement.cpp:21`
- 现状：PlayerState::GrantAbilitiesWithKit(bActivatePassives=true) 已授予即激活，被动 GA 注释同步更新。`ApplyGenderAbilityKit` 会先清理男女两套 Kit 的 Ability 与长期 GE 再授予当前套件，因此读档恢复、初始装配或角色切换重复调用都不会叠加永久被动。2026-08-22 分屏连续重建两次后 Armor/Medical/SmartAssist 仍各只有一份。

8. MedicalExpertise 设计补齐
- 关联 TODO：`主角-MedicalExpertise-Design`
- 位置：`ShootGA_Passive_MedicalExpertise.cpp:45`
- 目标：治疗拦截放大系数与不死窗口实现（医疗站已接入，其他治疗来源待补）。

9. SmartAssist 无人机 AI
- 关联 TODO：`主角-SmartAssist-DroneAI`
- 位置：`ShootGA_Passive_SmartAssist.cpp:41`
- 目标：无人机 Actor/AI 行为、Assist/自爆模式与数值配置化。

10. MarkHunter 击杀扩散
- 关联 TODO：`MarkHunter` 扩散逻辑
- 位置：`ShootGA_Passive_MarkHunter.cpp:98`
- 目标：击杀后扩散 Marked/易伤，区分精英/群体。
- 现状：击杀回血已按等级落地；Lv3 标记传播已落地（半径+50%可配置），传播表现与弱点高亮仍缺。

11. DeathPipeline 统一入口
- 关联 TODO：`主角-DeathPipeline-Refactor`
- 位置：`ShootAttributeSet.cpp:227`
- 目标：替换旧的 ICombatInterface::Execute_Die，接入新的死亡管线。

12. ShieldWall 动态数值注入
- 关联 TODO：`主角-ShieldWall-EffectSpecModifiers`
- 位置：`ShootSkillShieldWall.cpp:67`
- 现状：新增 ShieldWall SetByCaller GE，护盾/减伤数值由 ShieldWall 注入并设置持续时间。
- 目标：补齐表现与破碎逻辑（可选）。

13. MedicalStation HealEffect 兜底
- 关联 TODO：`主角-MedicalStation-HealEffect`
- 位置：`ShootSkillMedicalStation.cpp:141`
- 现状：HealEffectClass 为空时回退到通用即时治疗 GE。

14. 在线搜索 Presence 兼容
- 关联 TODO：`主角-Online-SEARCH_PRESENCE`
- 位置：`MultiplayerSessionsSubsystem.cpp:153`
- 现状：会话搜索同时写入 `SEARCH_PRESENCE` 与 `PRESENCESEARCH`，兼容新旧 Key。

15. 正史数值 TODO（文档对接）
- 关联 TODO：`Docs/SystemDesign/GameDesign/NumericalDesign/Skills/FemaleSkills_女主技能.md`
- 目标：将文档中的 TacticalScan/RescueCloak/RapidCharge/MedicalStation 等 TODO 逐项落地到代码与表现。

16. 救援交互组件接入倒地流程
- 关联 TODO：`主角-RescueCloak-Revive`
- 位置：`ShootAttributeSet.cpp:227` / 角色倒地流程（待明确）
- 现状：`AShootCharacter` 已监听 Health 变化，血量为 0 启用组件、血量恢复后关闭。
- 目标：明确倒地/死亡管线与表现（倒地动画、输入控制、救援窗口），并确认禁用时机。

17. 救援交互进度 UI 与时长消息
- 关联 TODO：`主角-RescueCloak-ReviveUI`
- 位置：`Interaction/LyraInteractionDurationMessage.h` / 交互 UI
- 现状：救援交互能力已广播 Duration 消息；PlayerController 已监听并广播蓝图委托。
- 目标：展示救援进度条与剩余时间；读取 EventMagnitude 修正耗时。

---

## 备注
- 本清单为主角技能与相关管线的执行入口，完成后需回写本表与对应任务文档。
