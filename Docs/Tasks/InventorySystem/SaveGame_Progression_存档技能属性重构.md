# 存档/技能/属性 架构重构 (评估与实施记录)

日期: 2026-08-23
状态: 代码已完成, 待编译+资产配置(Experience/GA_Grenade)后验证
相关: Docs/Tasks/InventorySystem/STATUS、Docs/Tasks/WeaponSystem/GrenadeHealthpack_实施记录.md

## 设计目标(用户决策)

- 技能 = 副本数据: 男/女主(含手雷)进入副本由 Experience 的 AbilitySet DA 授予, 出副本取回; 存档不持有技能(像 MOBA 每局从头开始)。
- 属性 = 副本数据: 每次进副本默认初始化; 回合制副本由 GameMode::InitializeRoundForAll 重置玩家与敌人属性并重授套件。
- 存档只存账号级战利品: Persistent 物品 / 资源 / 双主角外观+Quickbar; 移除 等级/XP/属性点/技能 字段(SaveVersion=3)。

## 旧代码问题(评估结论)

1. ShootSaveGame 的 PlayerLevel/XP/SpellPoints/AttributePoints/Strength/Vitality/Agility/Perception/SavedAbilities
   从未被 Capture 写入(死字段), 但 LoadProgress 的"非首次建档"分支从它们恢复:
   - 属性从存档恢复(InitializeDefaultAttributesFromSaveData, SetByCaller=存档值=0) -> MMC 派生 MaxHealth=0
   - 这正是"切换男女主血条归零"的根因(旧档/属性面板切换时旧属性值覆盖)。
2. 技能相关: AddCharacterAbilitiesFromSaveData(存档技能)+ PlayerState 性别数组(GrantAbilitiesWithKit)在"读档恢复/切换角色"时授予/移除 —— 切换与技能耦合, 且无存档时套件从未授予(手雷 T 无反应的另一个根因)。
3. 属性初始化系统本身是现成的: CharacterClassInfo(DataAsset)+ 角色蓝图默认 GE(DefaultPrimary/Secondary/VitalAttributes);
   无需另起炉灶, 只是不该与存档耦合。

## 实施清单(代码)

- ShootSaveGame.h: 删除 FSavedAbility 与 等级/XP/点数/主属性字段; SaveVersion=3; 注释说明。
- ShootSaveGameSubsystem::RestorePlayerInventoryState: 无存档也确定性别; 仅恢复 Persistent 物品/资源/Quickbar/外观;
  不再由存档授予技能(技能授予移交给 Experience; 若 Experience 已加载则由其生命周期授予)。
- ShootCharacter::LoadProgress: 简化为 每次进副本 InitializeDefaultAttributes + AddCharacterAbilities(基础能力);
  删除存档恢复分支。
- ShootAbilitySystemComponent/ShootAbilitySystemLibrary: 删除 AddCharacterAbilitiesFromSaveData / InitializeDefaultAttributesFromSaveData。
- AShootPlayerState: 删除男女 Active/Passive 数组(硬编码列表迁移到 DA_AbilitySet_ProtagonistMale/Female);
  ApplyGenderAbilityKit 改为从当前 Experience 的 AbilitySet(男/女)GiveToAbilitySystem(GenderAbilityHandles);
  TakeGenderAbilityKit 取回; SwitchToCharacter 不再即时切换技能(由 Experience 生命周期管理)。
- UShootExperienceDefinition: 新增 Male/FemaleProtagonistAbilitySet(TObjectPtr<const UShootAbilitySet>)。
- UShootExperienceManagerComponent: 加载 Experience 后 GrantPlayerGenderAbilities(补授/重授), 卸载时 TakePlayerGenderAbilities。
- AShootGameModeBase: InitializeRoundForAll(BlueprintCallable, 服务端): 重置所有玩家+敌人属性
  (AShootCharacterBase::ResetAndInitializeDefaultAttributes: 移除 SourceObject=this 的默认属性 GE 后重新初始化)
  并重授玩家当前性别套件。回合制副本由 GameMode 蓝图在每回合开始时调用。
- AShootCharacterBase: ResetAndInitializeDefaultAttributes。
- FProjectileWeaponConfig.ProjectileMesh + AShootProjectileBase.VisualMeshComponent(数据驱动可视网格, 手雷可见性修复)。
- UShootGA_ThrowGrenade: ThrowMontageMale/Female(EditDefaultsOnly)+ 按性别 Kit Tag 选择, 本地播放(手雷动画)。

## 待编译后执行的资产配置

- DA_Experience_DungeonTest: MaleProtagonistAbilitySet=DA_AbilitySet_ProtagonistMale; Female=...Female。
- GA_Grenade(蓝图): ThrowMontageMale=/Game/Characters/Heroes/CC/MM/.../AM_MM_Rifle_GrenadeToss;
  ThrowMontageFemale=/Game/Characters/Heroes/CC/MF/.../AM_MM_Rifle_GrenadeToss;
  ProjectileConfig.ProjectileMesh=/Game/Weapons/Grenade/Mesh/SM_grenade; TrailSystem=/Game/Effects/Particles/Explosion/NS_Grenade_Trail。
- 手雷爆炸 GameplayCue(Weapon.Grenade.Detonate)表现蓝图(NS_Grenade_Explosion/爆炸音效)待建。

## 验证项(编译后)

1. 进副本(TestMap_ListenServer/TestMap_SplitScreen)手雷 T 键: 动画/手雷可见/冷却 UI 启动。
2. 男主/女主技能 Q/E/C/X 可触发; 切换角色不再出现血条归零。
3. 出副本(Experience 卸载)技能消失; 存档仅含战利品。
4. 治疗包拾取即用; 满血无交互。
