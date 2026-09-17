# GameplayEffects 测试地图迁移记录

# 目标

- 把 HomeMap World Outliner 中 `GameplayEffects` 文件夹的调试 Actor 同步到：
  - `/Game/Maps/TestMap_ListenServer`
  - `/Game/Maps/TestMap_SplitScreen`
- 保留 Actor 类、Transform、Folder Path、Actor Tags 和可序列化实例属性，避免两张测试图继续维护不同的伤害、治疗、控制与交互调试入口。

# 迁移结果

- HomeMap 源文件夹共 24 个 Actor。
- TestMap_ListenServer 已同步 24 个 Actor。
- TestMap_SplitScreen 已同步 24 个 Actor。
- 两张目标地图重载后按稳定签名复核通过；签名包含 Actor Label、Class、Transform、Folder Path 和 Actor Tags。
- 迁移脚本为 `Scripts/Maps_MigrateGameplayEffects.py`，可重复运行；目标已有同签名对象时不会继续叠加。

# 实施边界

- 这次只迁移关卡实例，不改 GameplayEffect、GameplayAbility 或第三方插件源码。
- 目标地图原有非 `GameplayEffects` Actor 不在脚本管理范围内。
- 脚本在每次切换关卡前释放 Python 持有的 World/Actor 引用并执行 GC。UE 编辑器不允许 Python 跨 `LoadLevel` 持有旧 World；忽略这一点会触发 World Memory Leak 检查。

# 后续维护

- HomeMap 新增同类调试入口后，重新运行迁移脚本并检查三张地图数量和签名。
- 若某个 Actor 只适用于单机、Listen Server 或分屏，先在本文件记录例外，再从自动同步集合中排除；不要直接在目标地图手删后让脚本下次重新补回。
