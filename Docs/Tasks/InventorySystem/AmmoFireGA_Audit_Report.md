## AmmoFireGA_Audit_Report（开火 GA 弹药成本现状）

- UShootGA_Weapon_Fire_Projectile（Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_Projectile.cpp）
  - AdditionalCosts: UShootAbilityCost_AmmoTagStack ✅（构造确保存在，Tag=Inventory_Ammo_Magazine，Quantity=WeaponInstance.GetAmmoPerShot）
  - Legacy AmmoCost: 无
  - Extra ammo calls: 无（未调用 WeaponInstance::ConsumeAmmo*）

- UShootGA_Weapon_Fire_Rifle（Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_Rifle.cpp）
  - AdditionalCosts: UShootAbilityCost_AmmoTagStack ✅（构造追加，Tag=Inventory_Ammo_Magazine，Quantity=WeaponInstance.GetAmmoPerShot）
  - Legacy AmmoCost: 无
  - Extra ammo calls: 无

- UShootGA_Weapon_Fire_Sniper（Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_Sniper.cpp）
  - AdditionalCosts: UShootAbilityCost_AmmoTagStack ✅（构造追加，Tag=Inventory_Ammo_Magazine，Quantity=WeaponInstance.GetAmmoPerShot）
  - Legacy AmmoCost: 无
  - Extra ammo calls: 无

- UShootGA_Weapon_Fire_Shotgun（Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_Shotgun.cpp）
  - AdditionalCosts: UShootAbilityCost_AmmoTagStack ✅（构造追加，Tag=Inventory_Ammo_Magazine，Quantity=WeaponInstance.GetAmmoPerShot）
  - Legacy AmmoCost: 无
  - Extra ammo calls: 无

- UShootGA_Weapon_Fire_GrenadeLauncher（Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_GrenadeLauncher.cpp）
  - AdditionalCosts: UShootAbilityCost_AmmoTagStack ✅（构造追加，Tag=Inventory_Ammo_Magazine，Quantity=WeaponInstance.GetAmmoPerShot）
  - Legacy AmmoCost: 无
  - Extra ammo calls: 无

- UShootGA_Weapon_Fire_RocketLauncher（Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_RocketLauncher.cpp）
  - AdditionalCosts: UShootAbilityCost_AmmoTagStack ✅（构造追加，Tag=Inventory_Ammo_Magazine，Quantity=WeaponInstance.GetAmmoPerShot）
  - Legacy AmmoCost: 无
  - Extra ammo calls: 无

补充说明：
- 所有标准开火 GA（Projectile/Rifle/Sniper/Shotgun/GrenadeLauncher/RocketLauncher）现统一挂载 UShootAbilityCost_AmmoTagStack，弹药扣除走 Inventory_Ammo_Magazine + WeaponInstance.GetAmmoPerShot 路径；Overload 状态下 AmmoTagStack 会跳过扣弹。
- 项目中已删除 UShootAbilityCost_Ammo，未发现其他开火 GA 仍引用旧类。
