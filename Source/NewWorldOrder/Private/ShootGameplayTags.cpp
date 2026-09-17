// Copyright ZhaoYiJie


#include "ShootGameplayTags.h"

#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagsManager.h"

FShootGameplayTags FShootGameplayTags::GameplayTags;

void FShootGameplayTags::SetSetByCallerMagnitude(FGameplayEffectSpec& Spec, const FGameplayTag& DataTag,
	const float Magnitude)
{
	if (!DataTag.IsValid())
	{
		return;
	}

	// DataName 服务 C++ GE 的启动期安全配置；DataTag 服务蓝图 GE 与 ActiveEffect 快照。
	Spec.SetSetByCallerMagnitude(DataTag.GetTagName(), Magnitude);
	Spec.SetSetByCallerMagnitude(DataTag, Magnitude);
}

void FShootGameplayTags::InitializeNativeGameplayTags()
{
	/*
	 * Primary Attributes
	 */
	GameplayTags.Attributes_Primary_Strength = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Strength"),
		FString("Increases physical damage")
	);

	GameplayTags.Attributes_Primary_Vitality = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Vitality"),
		FString("Increases maximum health and health regeneration")
	);

	GameplayTags.Attributes_Primary_Agility = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Agility"),
		FString("Increases movement speed, attack speed, and dodge chance")
	);

	GameplayTags.Attributes_Primary_Perception = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Perception"),
		FString("Increases critical hit chance, accuracy, and vision range")
	);

	/*
	 * Secondary Attributes
	 */

	GameplayTags.Attributes_Secondary_Armor = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.Armor"),
		FString("Armor value, mainly used by enemies, reduces incoming damage")
	);

	GameplayTags.Attributes_Secondary_ArmorPenetration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.ArmorPenetration"),
		FString("Calculated from Strength (Strength × 0.8%), reduces enemy effective armor")
	);

	GameplayTags.Attributes_Secondary_CriticalHitChance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitChance"),
		FString("Calculated from Perception (Perception × 0.75%), chance to deal critical damage")
	);

	GameplayTags.Attributes_Secondary_CriticalHitDamage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitDamage"),
		FString("Base 1.5x multiplier for critical hits (v7.2)")
	);

	GameplayTags.Attributes_Secondary_CriticalHitResistance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitResistance"),
		FString("Critical hit resistance, mainly for elites and bosses, reduces actual critical hit chance")
	);

	GameplayTags.Attributes_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.MaxHealth"),
		FString("Calculated from Vitality using three-tier decreasing formula")
	);

	GameplayTags.Attributes_Secondary_ShieldCapacity = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.ShieldCapacity"),
		FString("Calculated from Vitality and MaxHealth: (20% + Vitality × 1%) × MaxHP")
	);

	GameplayTags.Attributes_Secondary_DamageReduction = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.DamageReduction"),
		FString("Calculated from Vitality: Vitality × 0.2%")
	);
	// Combat Buff Attributes（技能/被动临时加成）
	GameplayTags.Attributes_Combat_MoveSpeedMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.MoveSpeedMultiplier"),
		TEXT("Combat buff: move speed multiplier (base 1.0)"));
	GameplayTags.Attributes_Combat_FireRateMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.FireRateMultiplier"),
		TEXT("Combat buff: fire rate multiplier (base 1.0)"));
	GameplayTags.Attributes_Combat_ReloadSpeedMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.ReloadSpeedMultiplier"),
		TEXT("Combat buff: reload speed multiplier (base 1.0)"));
	GameplayTags.Attributes_Combat_HealingDoneMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.HealingDoneMultiplier"),
		TEXT("Combat buff: healing done multiplier (base 1.0)"));
	GameplayTags.Attributes_Combat_HealingReceivedMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.HealingReceivedMultiplier"),
		TEXT("Combat buff: healing received multiplier (base 1.0)"));
	GameplayTags.Attributes_Combat_ShieldCapacityBonus = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.ShieldCapacityBonus"),
		TEXT("Combat buff: extra shield capacity (additive)"));
	GameplayTags.Attributes_Combat_DamageReductionBonus = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.DamageReductionBonus"),
		TEXT("Combat buff: extra damage reduction (additive)"));
	// Ultimate charge
	GameplayTags.Attributes_Combat_UltimateCharge = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.UltimateCharge"),
		TEXT("Ultimate charge current value"));
	GameplayTags.Attributes_Combat_UltimateChargeMax = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Combat.UltimateChargeMax"),
		TEXT("Ultimate charge max value"));

	/*
	 * Input Tags
	 */
	GameplayTags.Abilities_Status_Eligible = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status.Eligible"),
		FString("Eligible Status")
	);

	GameplayTags.Abilities_Status_Equipped = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status.Equipped"),
		FString("Equipped Status")
	);

	GameplayTags.Abilities_Status_Locked = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status.Locked"),
		FString("Locked Status")
	);

	GameplayTags.Abilities_Status_Unlocked = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status.Unlocked"),
		FString("Unlocked Status")
	);

	GameplayTags.Abilities_Type_None = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Type.None"),
		FString("Type None")
	);

	GameplayTags.Abilities_Type_Offensive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Type.Offensive"),
		FString("Type Offensive")
	);

	GameplayTags.Abilities_Type_Passive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Type.Passive"),
		FString("Type Passive")
	);
	// Skill slot types
	GameplayTags.Ability_Type_Skill_Primary = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Skill.Primary"),
		TEXT("Primary skill slot (Q)"));
	GameplayTags.Ability_Type_Skill_Secondary = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Skill.Secondary"),
		TEXT("Secondary skill slot (E)"));
	GameplayTags.Ability_Type_Skill_Utility = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Skill.Utility"),
		TEXT("Utility skill slot (C)"));
	GameplayTags.Ability_Type_Skill_Ultimate = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Skill.Ultimate"),
		TEXT("Ultimate skill slot (X)"));
	GameplayTags.Ability_Type_Skill_Passive1 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Skill.Passive1"),
		TEXT("Passive slot 1"));
	GameplayTags.Ability_Type_Skill_Passive2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Skill.Passive2"),
		TEXT("Passive slot 2"));
	GameplayTags.Abilities_Kit_Protagonist_Male = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Kit.Protagonist.Male"),
		TEXT("Ability kit tag for male protagonist (grant/remove on gender switch)"));
	GameplayTags.Abilities_Kit_Protagonist_Female = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Kit.Protagonist.Female"),
		TEXT("Ability kit tag for female protagonist (grant/remove on gender switch)"));
	GameplayTags.Status_Marked = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Marked"),
		TEXT("Target is marked (tactical mark)"));
	GameplayTags.Status_Vulnerable = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Vulnerable"),
		TEXT("Target is vulnerable (bonus damage)"));
	GameplayTags.Status_ArmorBreak = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.ArmorBreak"),
		TEXT("Target armor reduced"));
	GameplayTags.Status_Overload = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Overload"),
		TEXT("Overload/ultimate buff active"));
	GameplayTags.Status_Cloaked = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Cloaked"),
		TEXT("Cloaked/invisible state (rescue cloak etc.)"));
	GameplayTags.Status_RescueSpeedBoost = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.RescueSpeedBoost"),
		TEXT("Rescue/revive speed boost while RescueCloak is active"));
	GameplayTags.Status_Stunned = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Stunned"),
		TEXT("Target is stunned"));
	GameplayTags.Status_Slowed = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Slowed"),
		TEXT("Target is slowed"));
	GameplayTags.Status_ImmuneDeath = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.ImmuneDeath"),
		TEXT("Target cannot be reduced below 1 HP while active"));
	GameplayTags.Status_MedicalExpertise_Lv2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.MedicalExpertise.Lv2"),
		TEXT("Medical Expertise level 2+ active"));
	GameplayTags.Status_MedicalExpertise_Lv3 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.MedicalExpertise.Lv3"),
		TEXT("Medical Expertise level 3 active"));
	/*
	 * Input Tags
	 */

	GameplayTags.InputTag_LMB = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.LMB"),
		FString("Input Tag for Left Mouse Button")
	);

	GameplayTags.InputTag_RMB = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.RMB"),
		FString("Input Tag for Right Mouse Button")
	);

	GameplayTags.InputTag_Q = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Q"),
		FString("Input Tag for Q key / Primary skill")
	);

	GameplayTags.InputTag_E = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.E"),
		FString("Input Tag for E key / Secondary skill")
	);

	GameplayTags.InputTag_C = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.C"),
		FString("Input Tag for C key / Utility skill")
	);

	GameplayTags.InputTag_X = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.X"),
		FString("Input Tag for X key / Ultimate skill")
	);

	GameplayTags.InputTag_Ability_Interact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Ability.Interact"),
		FString("Input Tag for Ability Interact")
	);

	// 切枪语义独立于具体输入设备；键鼠和手柄按键由 InputConfig/IMC 配置。
	GameplayTags.InputTag_Weapon_Next = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Weapon.Next"),
		FString("Input Tag for cycling to the next RuntimeOnly weapon slot")
	);

	GameplayTags.InputTag_Weapon_Previous = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Weapon.Previous"),
		FString("Input Tag for cycling to the previous RuntimeOnly weapon slot")
	);

	GameplayTags.InputTag_Weapon_Drop = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Weapon.Drop"),
		FString("Input Tag for dropping the active RuntimeOnly weapon")
	);

	GameplayTags.InputTag_Camera_TogglePerspective = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Camera.TogglePerspective"),
		FString("Toggle the local player's Experience-controlled camera perspective")
	);


	GameplayTags.InputTag_Space = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Space"),
		FString("Input Tag for Space key")
	);

	GameplayTags.InputTag_R = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.R"),
		FString("Input Tag for R key")
	);

	GameplayTags.InputTag_Passive_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Passive.1"),
		FString("Input Tag Passive Ability 1")
	);

	GameplayTags.InputTag_Passive_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Passive.2"),
		FString("Input Tag Passive Ability 2")
	);

	/*
	 * Meta Attributes
	 */

	GameplayTags.Attributes_Meta_IncomingXP = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Meta.IncomingXP"),
		FString("Incoming XP Meta Attribute")
	);

	GameplayTags.Abilities_Passive_ListenForEvent = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Passive.ListenForEvent"),
		FString("Abilities Passive ListenForEvent")
	);

	/*
	 * Player Tags
	 */

	GameplayTags.Player_Block_CursorTrace = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.CursorTrace"),
		FString("Block tracing under the cursor")
	);

	GameplayTags.Player_Block_InputHeld = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputHeld"),
		FString("Block Input Held callback for input")
	);

	GameplayTags.Player_Block_InputPressed = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputPressed"),
		FString("Block Input Pressed callback for input")
	);

	GameplayTags.Player_Block_InputReleased = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputReleased"),
		FString("Block Input Released callback for input")
	);


	GameplayTags.Ability_Type_Action_Jump = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Action.Jump"),
		FString("Ability Type Action Jump")
	);

	GameplayTags.Ability_Skill_RobotCompanion = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Skill.RobotCompanion"),
		FString("Player Match Skill that summons or commands the robot companion")
	);
	GameplayTags.Ability_Skill_Robot_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Skill.RobotCompanion.Fire"),
		FString("Robot-owned server ability that performs one weapon shot")
	);
	GameplayTags.Ability_Skill_Robot_Melee = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Skill.RobotCompanion.Melee"),
		FString("Robot-owned server ability that performs a close-range claw or chomp attack")
	);
	GameplayTags.Ability_Mode_Robot_Ranged = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Mode.Robot.Ranged"), FString("Robot keeps distance and uses rapid low-damage fire"));
	GameplayTags.Ability_Mode_Robot_Melee = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Mode.Robot.Melee"), FString("Robot closes distance and uses slower high-damage melee attacks"));
	GameplayTags.Ability_Mode_Robot_Balanced = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Mode.Robot.Balanced"), FString("Robot escorts its owner and chooses melee or ranged attacks by distance"));
	GameplayTags.Cooldown_Skill_RobotCompanion = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Cooldown.Skill.RobotCompanion"), FString("Robot resummon cooldown after combat destruction"));


	GameplayTags.SetByCaller_Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.Damage"),
		FString("SetByCaller tag used by damage gameplay effects.")
	);

	GameplayTags.SetByCaller_Heal = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.Heal"),
		FString("SetByCaller tag used by healing gameplay effects.")
	);

	GameplayTags.SetByCaller_ShieldCapacityBonus = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.ShieldCapacityBonus"),
		FString("SetByCaller tag used by shield capacity bonus gameplay effects.")
	);

	GameplayTags.SetByCaller_DamageReductionBonus = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.DamageReductionBonus"),
		FString("SetByCaller tag used by damage reduction bonus gameplay effects.")
	);

	GameplayTags.SetByCaller_HealthSnapshotDelta = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.HealthSnapshotDelta"),
		FString("SetByCaller tag used by snapshot restore to apply delta to Health.")
	);

	GameplayTags.SetByCaller_ShieldSnapshotDelta = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.ShieldSnapshotDelta"),
		FString("SetByCaller tag used by snapshot restore to apply delta to ShieldCapacity.")
	);

	GameplayTags.SetByCaller_UltimateChargeSnapshotDelta = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.UltimateChargeSnapshotDelta"),
		FString("SetByCaller tag used by snapshot restore to apply delta to UltimateCharge.")
	);
	GameplayTags.SetByCaller_HealingDoneMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.HealingDoneMultiplier"),
		FString("SetByCaller tag used by healing done multiplier.")
	);
	GameplayTags.SetByCaller_HealingReceivedMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.HealingReceivedMultiplier"),
		FString("SetByCaller tag used by healing received multiplier.")
	);
	GameplayTags.SetByCaller_MoveSpeedMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.MoveSpeedMultiplier"),
		FString("SetByCaller tag used by move speed multiplier.")
	);
	GameplayTags.SetByCaller_ReloadSpeedMultiplier = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("SetByCaller.ReloadSpeedMultiplier"),
		FString("SetByCaller tag used by reload speed multiplier.")
	);

	GameplayTags.Ability_Weapon_NoFiring = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Weapon.NoFiring"),
		FString("Weapon fire will be blocked/canceled if the player has this tag.")
	);

	GameplayTags.Ability_Interaction_Collect = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Interaction.Collect"),
		FString("Ability used when the player collects a pickup via interaction.")
	);

	GameplayTags.Gameplay_Zone_WeakSpot = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Gameplay.Zone.WeakSpot"),
		FString("headshot or other WeakSpot.")
	);
	GameplayTags.GameplayCue_Character_DamageTaken = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Character.DamageTaken"),
		FString("Actual Health damage feedback for the damage source and target presentation.")
	);
	GameplayTags.GameplayCue_Skill_Robot_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Skill.RobotCompanion.Fire"), FString("Replicated robot muzzle and shot feedback"));
	GameplayTags.GameplayCue_Skill_Robot_Melee = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Skill.RobotCompanion.Melee"), FString("Replicated robot melee impact feedback"));
	GameplayTags.GameplayCue_Skill_Robot_SelfDestruct = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Skill.RobotCompanion.SelfDestruct"), FString("Robot owner-loss self-destruct feedback"));
	GameplayTags.GameplayCue_Skill_Robot_SummonImpact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Skill.RobotCompanion.SummonImpact"), FString("Robot drop summon landing feedback"));
	GameplayTags.GameplayCue_Skill_TacticalOverload_Active = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Skill.TacticalOverload.Active"), FString("Looping Tactical Overload presentation"));

	GameplayTags.Ability_ActivateFail_Cost = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.ActivateFail.Cost"),
		FString("Ability Activate Fail Cost")
	);

	GameplayTags.GameplayCue_Weapon_Rifle_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Weapon.Rifle.Fire"),
		FString("Gameplay Cue for Rifle Fire")
	);
	GameplayTags.GameplayCue_Weapon_Rifle_Impact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Weapon.Rifle.Impact"),
		FString("Gameplay Cue for Rifle Impact")
	);

	// 在 InitializeNativeGameplayTags 函数中添加初始化代码
	GameplayTags.Ability_ActivateFail_MagazineFull = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.ActivateFail.MagazineFull"),
		FString("Ability activate fail due to magazine being full")
	);

	GameplayTags.Ability_ActivateFail_NoSpareAmmo = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.ActivateFail.NoSpareAmmo"),
		FString("Ability activate fail due to no spare ammo")
	);

	GameplayTags.GameplayEvent_ReloadDone = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayEvent.ReloadDone"),
		FString("Gameplay event for when reload is done")
	);
	GameplayTags.GameplayEvent_Rescue_Completed = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayEvent.Rescue.Completed"),
		FString("Gameplay event fired when revive/rescue interaction completed")
	);

	GameplayTags.Event_Movement_WeaponFire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.Movement.WeaponFire"),
		FString("Event for weapon firing movement")
	);
	// 在 InitializeNativeGameplayTags 函数中添加初始化代码
	GameplayTags.Event_Movement_Reload = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.Movement.Reload"),
		FString("Event for reload movement")
	);
	GameplayTags.Event_Movement_ADS = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.Movement.ADS"),
		FString("Event for aim-down-sights movement and animation state")
	);

	GameplayTags.Ability_Type_Action_Reload = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Action.Reload"),
		FString("Ability type for reload action")
	);
	GameplayTags.Ability_Type_Action_ADS = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Action.ADS"),
		FString("Ability type for hold-to-aim action")
	);
	GameplayTags.Ability_Type_Passive_AutoReload = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Type.Passive.AutoReload"),
		FString("Passive ability that activates the equipped weapon reload when its magazine is empty")
	);

	// 在 InitializeNativeGameplayTags() 填充（ShootGameplayTags.cpp）
	GameplayTags.Msg_Quickbar_SlotsChanged       = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Message.Quickbar.SlotsChanged"), TEXT("Quickbar slots changed snapshot"));
	GameplayTags.Msg_Quickbar_ActiveIndexChanged = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Message.Quickbar.ActiveIndexChanged"), TEXT("Quickbar active slot index changed"));
	GameplayTags.Msg_Weapon_AmmoChanged          = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Message.Weapon.AmmoChanged"), TEXT("Weapon ammo/reserve changed"));
	GameplayTags.Msg_UI_Reticle_HitNotify        = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Message.UI.Reticle.HitNotify"), TEXT("Reticle hit marker notification (screen-space markers)"));
	GameplayTags.Msg_UI_Reticle_ADS              = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Message.UI.Reticle.ADS"), TEXT("Reticle ADS state toggles (bIsAds payload)"));
	GameplayTags.Msg_UI_Reticle_Elimination      = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Message.UI.Reticle.Elimination"), TEXT("Reticle elimination feedback message"));
	GameplayTags.Msg_Skill_LoadoutChanged        = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Message.Skill.LoadoutChanged"), TEXT("Owning player's four-slot Match Skill snapshot changed"));
	GameplayTags.HUD_Slot_Reticle                = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("HUD.Slot.Reticle"), TEXT("Per-local-player reticle extension point"));
	GameplayTags.HUD_Slot_Quickbar               = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("HUD.Slot.Quickbar"), TEXT("Per-local-player quickbar extension point"));
	GameplayTags.HUD_Slot_Interaction            = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("HUD.Slot.Interaction"), TEXT("Per-local-player interaction hold progress extension point"));
	GameplayTags.HUD_Slot_Skills                 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("HUD.Slot.Skills"), TEXT("Per-local-player four-slot Match Skill extension point"));
	GameplayTags.HUD_Slot_StatusEffects          = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("HUD.Slot.StatusEffects"), TEXT("Per-local-player timed status effect extension point"));

	// 逐发装填相关（可与动画通知对应）
	GameplayTags.GameplayEvent_Reload_InsertShell = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayEvent.Reload.InsertShell"), TEXT("Insert one shell during reload"));
	GameplayTags.GameplayEvent_Reload_Complete = GameplayTags.GameplayEvent_ReloadDone; // 若你愿意复用

	/*
	 * 库存系统相关 Tags（Inventory System）
	 */

	// 库存变化消息（用于 UI 更新）
	GameplayTags.Inventory_Message_StackChanged = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Message.StackChanged"),
		FString("Inventory stack count changed (for UI updates via GameplayMessageSubsystem)")
	);

	/*
	 * 材料类型 Tags
	 */

	// 军用合金（传说武器核心材料，堆栈上限 999）
	GameplayTags.Inventory_Material_MilitaryAlloy = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Material.MilitaryAlloy"),
		FString("Military Alloy - Core material for legendary weapons, stack limit 999")
	);

	// 服装碎片（制作服装的材料，堆栈上限 9999）
	GameplayTags.Inventory_Material_ClothingFragments = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Material.ClothingFragments"),
		FString("Clothing Fragments - Material for crafting clothing, stack limit 9999")
	);

	// 金币（通用货币，堆栈上限 99999）
	GameplayTags.Inventory_Material_Gold = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Material.Gold"),
		FString("Gold - Universal currency, stack limit 99999")
	);

	/*
	 * 弹药类型 Tags
	 */

	// 步枪弹药
	GameplayTags.Inventory_Ammo_Rifle = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Ammo.Rifle"),
		FString("Rifle Ammo - Reserve ammunition for rifles")
	);

	// 霰弹枪弹药
	GameplayTags.Inventory_Ammo_Shotgun = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Ammo.Shotgun"),
		FString("Shotgun Ammo - Reserve ammunition for shotguns")
	);

	// 手枪弹药
	GameplayTags.Inventory_Ammo_Pistol = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Ammo.Pistol"),
		FString("Pistol Ammo - Reserve ammunition for pistols")
	);

	// 狙击枪弹药
	GameplayTags.Inventory_Ammo_Sniper = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Ammo.Sniper"),
		FString("Sniper Ammo - Reserve ammunition for sniper rifles")
	);

	GameplayTags.Inventory_Ammo_Magazine = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Ammo.Magazine"),
		FString("Current magazine/ammo in clip for equipped weapon")
	);

	GameplayTags.Inventory_Ammo_Reserve = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Ammo.Reserve"),
		FString("Reserve ammo carried for equipped weapon")
	);

	GameplayTags.Inventory_Ammo_MagazineCapacity = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Ammo.MagazineCapacity"),
		FString("Maximum magazine ammo for an inventory weapon instance")
	);

	GameplayTags.Inventory_Ammo_ReserveCapacity = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Ammo.ReserveCapacity"),
		FString("Maximum reserve ammo for an inventory weapon instance")
	);

	// 账号资源变化消息（材料/货币/徽章/设计图）
	GameplayTags.Inventory_Resource_Message_Changed = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Inventory.Resource.Message.Changed"),
		FString("Resource (materials/currency/key items/blueprints) changed message")
	);

	GameplayTags.UI_Toast_ResourcePickup = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("UI.Toast.ResourcePickup"),
		FString("HUD toast message triggered when materials/currencies are picked up")
	);

	GameplayTags.Faction_Player = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Faction.Player"),
		FString("Applied to player controlled avatars for interaction filtering/faction based effects")
	);

	GameplayTags.Faction_Enemy = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Faction.Enemy"),
		FString("Applied to hostile AI avatars for interaction filtering/faction based effects")
	);

	GameplayTags.GameplayCue_Interaction_Pickup = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.Interaction.Pickup"),
		FString("Cue triggered when a pickup interaction completes")
	);

}
