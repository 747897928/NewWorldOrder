# 迁移计划

# 阶段 1：基线审计与测试护栏

- 搜索所有能力授予、移除、激活和存档恢复入口。
- 用 MCP 搜索 Character、PlayerState、GameMode、GameState 和 AbilitySet 的蓝图子类与默认值。
- 记录所有正式 AbilitySet、EquipmentDefinition、InputConfig 和 SaveGame 资产引用。
- Session + CommonUI 生命周期已拆分到 `Docs/Tasks/SessionUI/`，本阶段只核对其与未来 Experience 选择的接口边界。
- 添加 AbilitySpec 数量、来源、SourceObject 和激活状态的开发期诊断。
- 建立首次进入、重生、切换角色、装备、交互和读档的自动化或稳定 PIE 验收表。
- 修订 `Docs/SystemDesign/GameFlow/Overview_总览.md` 中本地分屏存档冲突。

退出条件：每个现有授予入口有分类和证据，未发现的蓝图引用风险已清零或列入阻塞项。

# 阶段 2：统一 AbilitySet

- 扩展现有 `UShootAbilitySet` 所需的授予策略和句柄簿记，不新建平行 AbilitySet 类型。
- 创建基础玩家、男主、女主 AbilitySet 资产。
- 将 Jump、Interact、男女主主动和被动迁入资产。
- 保持 Equipment AbilitySet 主线不变。
- 暂时保留旧启动数组和 Kit 入口，但加入互斥开关，只允许新旧一条链生效。
- 新链路验收后删除旧启动数组、C++ 硬编码技能数组和重复授予入口。

退出条件：首次进入、重生和角色切换均由 AbilitySet 完成，AbilitySpec 不重复，被动效果可撤销。

# 阶段 3：引入 PawnData

- 新增最小 `UShootPawnData`。
- 创建男女主 TPS PawnData。
- 把 PawnClass、InputConfig、相机配置、基础 AbilitySet 和标签关系迁入 PawnData。
- PlayerState 保存已验证的 PawnData 选择或稳定角色标识，不保存裸资产路径字符串。
- 角色切换后端改为切换 PawnData 生命周期。

退出条件：两个主角和至少两个本地玩家能使用独立 PawnData，UI 和输入不串玩家。

# 阶段 4：引入 Experience

- 新增最小 ExperienceDefinition 和 GameState ExperienceManager。
- 先落地 CampaignSolo，再落地 SplitScreen 和 Online。
- 将 HUD 插槽、默认 PawnData 和模式 AbilitySet 接入 Experience。
- 验证服务器选择、客户端等待、旅行和重生时序。
- 接入 `Docs/Tasks/SessionUI/` 已完成的会话请求与 MainMenu，不在 Experience 阶段重新实现 Session UI。
- Session 请求未来可追加 Experience 标识；当前 Session UI 任务先以 HomeMap 大厅流程完成闭环。
- 不在本阶段引入 GameFeature 插件拆分。

退出条件：三种战役入口共用同一加载状态机，主机和客户端 Experience 一致。

# 阶段 5：存档版本迁移

- 新增存档版本号和一次性升级函数。
- 把 `SavedAbilities` 转为解锁、等级和槽位选择。
- 明确本地分屏共享战役存档与每个 LocalPlayer 私有设置的边界。
- 验证旧存档库存、资源、衣柜、男女主 Loadout 和外观不丢失。
- 新结构稳定后删除 Aura AbilitySpec 恢复链。

退出条件：旧存档升级可重复测试，新存档不再依赖运行时 AbilitySpec 序列化。

# 阶段 6：模式扩展验证

- 新增 Infection Experience 和 Infected PawnData 的最小垂直切片。
- 验证运行时阵营切换、能力撤销、RuntimeOnly 清理和 HUD 切换。
- 验证 TPS/FPS 相机配置能复用角色业务能力。
- 只有出现真实按需加载或独立发布需求时，才创建 GameFeature 插件试点。

退出条件：新增模式不修改基础 Character/PlayerState 的模式 if/else 主线。

# 阶段 7：删除 Aura 遗留

- 再次搜索 C++、蓝图、DataAsset、Config 和 SaveGame 引用。
- 删除 CharacterClassInfo、旧启动数组、旧技能存档恢复和废弃标签中已确认无引用部分。
- 删除仅为旧链路存在的兼容函数、空事件和隐藏资产。
- 更新 AGENTS、SystemDesign、ContextPack、QuickReference 和任务状态。

退出条件：项目只有一条能力初始化主线，完整编译、Cook、单人、分屏和在线验收通过。

# 每阶段提交规则

- 每阶段单独提交，不把资产迁移、存档升级和遗留删除混在一个不可回退提交中。
- 先提交替代链路，再提交旧链路删除。
- 每次资产修改后编译并保存相关蓝图和 DataAsset。
- 每次 C++ 修改执行 `Scripts/Build_Windows.ps1`。
- 每阶段推送远程并在 `STATUS.md` 记录 commit 和验收结论。
