// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FGameplayEffectSpec;

/**
 * AuraGameplayTags
 *
 * Singleton containing native Gameplay Tags
 */

struct FShootGameplayTags
{
public:
	static const FShootGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();

	/**
	 * 同时写入 GameplayTag 与同名 FName 通道。
	 * C++ GameplayEffect CDO 必须使用 DataName，避免早于 AssetManager 初始化时把 DataTag 固化为 None；
	 * Tag 通道仍保留给蓝图 GE 与角色切换的 ActiveEffect 快照。
	 */
	static void SetSetByCallerMagnitude(FGameplayEffectSpec& Spec, const FGameplayTag& DataTag, float Magnitude);
	
	//Strength 力量，影响武器伤害加成和护甲穿透
	FGameplayTag Attributes_Primary_Strength;

	//Vitality 活力，生命力
	//生命力，影响最大生命值,护盾容量上限和减伤
	FGameplayTag Attributes_Primary_Vitality;

	//Agility 敏捷，灵活性
	//敏捷，影响移动速度、换弹速度、射击移速惩罚减免、翻滚充能次数
	FGameplayTag Attributes_Primary_Agility;

	//Perception 感知，洞察力
	//感知，影响暴击率(爆头和暴击是两个独立的伤害乘区，可以同时生效)、弱点伤害倍率
	FGameplayTag Attributes_Primary_Perception;

	//护甲，主要用于敌人，减少受到的伤害
	FGameplayTag Attributes_Secondary_Armor;
	//护甲穿透，从Strength计算（Strength × 0.8%），减少敌人有效护甲
	FGameplayTag Attributes_Secondary_ArmorPenetration;
	//暴击几率，从Perception计算（Perception × 0.75%）
	FGameplayTag Attributes_Secondary_CriticalHitChance;
	//暴击伤害，基础1.5倍（v7.2）
	FGameplayTag Attributes_Secondary_CriticalHitDamage;
	//暴击抗性，主要用于精英和BOSS，减少实际暴击率
	FGameplayTag Attributes_Secondary_CriticalHitResistance;

	//最大生命值，从Vitality计算（三段递减公式）
	FGameplayTag Attributes_Secondary_MaxHealth;
	//护盾容量，从Vitality和MaxHealth计算：(20% + Vitality × 1%) × MaxHP
	FGameplayTag Attributes_Secondary_ShieldCapacity;
	//减伤，从Vitality计算：Vitality × 0.2%
	FGameplayTag Attributes_Secondary_DamageReduction;
	// 战斗临时属性（技能/被动 Buff，武器不挂 ASC）
	FGameplayTag Attributes_Combat_MoveSpeedMultiplier;
	FGameplayTag Attributes_Combat_FireRateMultiplier;
	FGameplayTag Attributes_Combat_ReloadSpeedMultiplier;
	FGameplayTag Attributes_Combat_HealingDoneMultiplier;
	FGameplayTag Attributes_Combat_HealingReceivedMultiplier;
	FGameplayTag Attributes_Combat_ShieldCapacityBonus;
	FGameplayTag Attributes_Combat_DamageReductionBonus;
	// 大招充能
	FGameplayTag Attributes_Combat_UltimateCharge;
	FGameplayTag Attributes_Combat_UltimateChargeMax;

	FGameplayTag Attributes_Meta_IncomingXP;
	FGameplayTag Abilities_Passive_ListenForEvent;

	FGameplayTag Abilities_Status_Locked;
	FGameplayTag Abilities_Status_Eligible;
	FGameplayTag Abilities_Status_Unlocked;
	FGameplayTag Abilities_Status_Equipped;

	FGameplayTag Abilities_Type_Offensive;
	FGameplayTag Abilities_Type_Passive;
	FGameplayTag Abilities_Type_None;
	FGameplayTag Ability_Type_Skill_Primary;
	FGameplayTag Ability_Type_Skill_Secondary;
	FGameplayTag Ability_Type_Skill_Utility;
	FGameplayTag Ability_Type_Skill_Ultimate;
	FGameplayTag Ability_Type_Skill_Passive1;
	FGameplayTag Ability_Type_Skill_Passive2;
	// Gender kits：授予/清理男女主套件用
	FGameplayTag Abilities_Kit_Protagonist_Male;
	FGameplayTag Abilities_Kit_Protagonist_Female;
	// 状态 Tags（核心技能用）
	FGameplayTag Status_Marked;
	FGameplayTag Status_Vulnerable;
	FGameplayTag Status_ArmorBreak;
	FGameplayTag Status_Overload;
	FGameplayTag Status_Cloaked;
	FGameplayTag Status_RescueSpeedBoost;
	FGameplayTag Status_Stunned;
	FGameplayTag Status_Slowed;
	FGameplayTag Status_ImmuneDeath;
	FGameplayTag Status_MedicalExpertise_Lv2;
	FGameplayTag Status_MedicalExpertise_Lv3;

	FGameplayTag InputTag_LMB;
	FGameplayTag InputTag_RMB;
	FGameplayTag InputTag_Q;
	FGameplayTag InputTag_E;
	FGameplayTag InputTag_C;
	FGameplayTag InputTag_X;
	FGameplayTag InputTag_Passive_1;
	FGameplayTag InputTag_Passive_2;
	FGameplayTag InputTag_Space;
	FGameplayTag InputTag_R;
	FGameplayTag InputTag_Ability_Interact;
	FGameplayTag InputTag_Weapon_Next;
	FGameplayTag InputTag_Weapon_Previous;
	FGameplayTag InputTag_Weapon_Drop;
	FGameplayTag InputTag_Camera_TogglePerspective;

	FGameplayTag Player_Block_InputPressed;
	FGameplayTag Player_Block_InputHeld;
	FGameplayTag Player_Block_InputReleased;
	FGameplayTag Player_Block_CursorTrace;

	FGameplayTag Ability_Type_Action_Jump;

	FGameplayTag Ability_Type_Action_Skill1;

	FGameplayTag Ability_Type_Action_Skill2;

	FGameplayTag Ability_Type_Action_Skill3;

	FGameplayTag Ability_Type_Action_Skill4;

	FGameplayTag Ability_Weapon_NoFiring;
	FGameplayTag Ability_Interaction_Collect;
	FGameplayTag Ability_Skill_RobotCompanion;
	FGameplayTag Ability_Skill_Robot_Fire;
	FGameplayTag Ability_Skill_Robot_Melee;
	FGameplayTag Ability_Mode_Robot_Ranged;
	FGameplayTag Ability_Mode_Robot_Melee;
	FGameplayTag Ability_Mode_Robot_Balanced;
	FGameplayTag Cooldown_Skill_RobotCompanion;

	FGameplayTag SetByCaller_Damage;

	FGameplayTag SetByCaller_Heal;

	FGameplayTag SetByCaller_ShieldCapacityBonus;

	FGameplayTag SetByCaller_DamageReductionBonus;

	FGameplayTag SetByCaller_HealthSnapshotDelta;

	FGameplayTag SetByCaller_ShieldSnapshotDelta;

	FGameplayTag SetByCaller_UltimateChargeSnapshotDelta;
	FGameplayTag SetByCaller_HealingDoneMultiplier;
	FGameplayTag SetByCaller_HealingReceivedMultiplier;
	FGameplayTag SetByCaller_MoveSpeedMultiplier;
	FGameplayTag SetByCaller_ReloadSpeedMultiplier;

	FGameplayTag Gameplay_Zone_WeakSpot;
	FGameplayTag GameplayCue_Character_DamageTaken;
	FGameplayTag GameplayCue_Skill_Robot_Fire;
	FGameplayTag GameplayCue_Skill_Robot_Melee;
	FGameplayTag GameplayCue_Skill_Robot_SelfDestruct;
	FGameplayTag GameplayCue_Skill_Robot_SummonImpact;
	FGameplayTag GameplayCue_Skill_TacticalOverload_Active;

	FGameplayTag Ability_ActivateFail_Cost;

	FGameplayTag GameplayCue_Weapon_Rifle_Fire;

	FGameplayTag GameplayCue_Weapon_Rifle_Impact;

	FGameplayTag Ability_ActivateFail_MagazineFull;

	FGameplayTag Ability_ActivateFail_NoSpareAmmo;

	FGameplayTag GameplayEvent_ReloadDone;
	FGameplayTag GameplayEvent_Rescue_Completed;

	FGameplayTag Event_Movement_WeaponFire;

	FGameplayTag Event_Movement_Reload;
	FGameplayTag Event_Movement_ADS;

	FGameplayTag Ability_Type_Action_Reload;
	FGameplayTag Ability_Type_Action_ADS;
	FGameplayTag Ability_Type_Passive_AutoReload;

	// 在 FShootGameplayTags 里增加字段（ShootGameplayTags.h）
	FGameplayTag Msg_Quickbar_SlotsChanged;
	FGameplayTag Msg_Quickbar_ActiveIndexChanged;
	FGameplayTag Msg_Weapon_AmmoChanged;
	FGameplayTag Msg_UI_Reticle_HitNotify;
	FGameplayTag Msg_UI_Reticle_ADS;
	FGameplayTag Msg_UI_Reticle_Elimination;
	FGameplayTag Msg_Skill_LoadoutChanged;
	FGameplayTag HUD_Slot_Reticle;
	FGameplayTag HUD_Slot_Quickbar;
	FGameplayTag HUD_Slot_Interaction;
	FGameplayTag HUD_Slot_Skills;
	FGameplayTag HUD_Slot_StatusEffects;

	//（可选，用于逐发装填）
	FGameplayTag GameplayEvent_Reload_InsertShell;
	FGameplayTag GameplayEvent_Reload_Complete; // 可复用你已有的 GameplayEvent_ReloadDone

	/*
	 * 库存系统相关 Tags（Inventory System）
	 */

	// 库存变化消息（Inventory Change Message）
	// 用于通过 GameplayMessageSubsystem 广播库存变化事件
	// UI 可以监听此 Tag 以更新背包、制作界面等
	FGameplayTag Inventory_Message_StackChanged;

	/*
	 * 材料类型 Tags（Material Types）
	 * 用于 StatTags 存储材料数量
	 * 示例：Instance->AddStatTagStack(Inventory_Material_MilitaryAlloy, 50)
	 */

	// 军用合金（Military Alloy）
	// 传说武器核心材料，堆栈上限 999
	FGameplayTag Inventory_Material_MilitaryAlloy;

	// 服装碎片（Clothing Fragments）
	// 制作服装的材料，堆栈上限 9999
	FGameplayTag Inventory_Material_ClothingFragments;

	// 金币（Gold）
	// 通用货币，堆栈上限 99999
	FGameplayTag Inventory_Material_Gold;

	/*
	 * 弹药类型 Tags（Ammo Types）
	 * 用于 StatTags 存储弹药数量
	 * 示例：Instance->AddStatTagStack(Inventory_Ammo_Rifle, 120)
	 */

	// 步枪弹药（Rifle Ammo）
	FGameplayTag Inventory_Ammo_Rifle;

	// 霰弹枪弹药（Shotgun Ammo）
	FGameplayTag Inventory_Ammo_Shotgun;

	// 手枪弹药（Pistol Ammo）
	FGameplayTag Inventory_Ammo_Pistol;

	// 狙击枪弹药（Sniper Ammo）
	FGameplayTag Inventory_Ammo_Sniper;

	// 通用弹夹、备弹及容量 Tag（武器实例 StatTags 使用）
	FGameplayTag Inventory_Ammo_Magazine;
	FGameplayTag Inventory_Ammo_Reserve;
	FGameplayTag Inventory_Ammo_MagazineCapacity;
	FGameplayTag Inventory_Ammo_ReserveCapacity;

	// 资源变化消息（材料/货币/徽章/设计图等账号资源）
	FGameplayTag Inventory_Resource_Message_Changed;

	// HUD Toast：材料拾取提示（+10 军用合金）
	FGameplayTag UI_Toast_ResourcePickup;

	// 阵营 Tags（交互/拾取效果使用）
	FGameplayTag Faction_Player;
	FGameplayTag Faction_Enemy;

	// GameplayCue：拾取完成特效
	FGameplayTag GameplayCue_Interaction_Pickup;

private:
	static FShootGameplayTags GameplayTags;
};
