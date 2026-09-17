# Iris Replication 快速笔记

更新：2025-11-16  
整理：Codex

## 1. Iris 是什么
- UE5 引入的新一代网络复制框架，替代传统 `NetDriver + ReplicationGraph`。
- 核心思想：把可复制对象拆成多个 **Replication Fragments**，Iris 负责 diff/压缩/调度发送。
- 优势：带宽管理更细粒度、支持对象聚合、延迟更低，Lyra 及 UE5 示例项目默认启用。

## 2. 什么时候会用到 Iris API
- UObject 想作为“子对象”复制，比如 `UShootEquipmentInstance`、Lyra 的 WeaponInstance：
  1. 覆写 `RegisterReplicationFragments(FFragmentRegistrationContext&, EFragmentRegistrationFlags)`。
  2. 在实现里调用 `UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(...)`。
- 只有当项目启用 Iris（UE5 默认），上述 API 才生效。

## 3. 构建 / 模块依赖
- `FReplicationFragmentUtil` 位于 `IrisCore` 模块，若 Build.cs 没声明依赖就会在 **链接阶段** 报 `LNK2019`（无法解析 `CreateAndRegisterFragmentsForObject`）。
- 本项目：`NewWorldOrder.Build.cs` 的 `PrivateDependencyModuleNames` 已加入 `"IrisCore"`（2025-11-16 更新）。
- 记忆法：
  - 只要类里 `#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"` 或覆写 `RegisterReplicationFragments`，就必须链接 IrisCore。

## 4. 实现流程示例（`UShootEquipmentInstance`）
```cpp
#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"

void UShootEquipmentInstance::RegisterReplicationFragments(
    UE::Net::FFragmentRegistrationContext& Context,
    UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
    UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(
        this, Context, RegistrationFlags);
}
#endif
```
- `UE_WITH_IRIS` 宏用于兼容关闭 Iris 的情况。
- 需要在构建环境里开启 `WITH_IRIS=1`（UE5 默认）才能进入上述代码。

## 5. 调试 / 排坑
1. **编译能过，链接炸 (`LNK2019`)**  
   - 大概率缺少 `IrisCore` 依赖，检查 Build.cs → `PrivateDependencyModuleNames`。
2. **运行时报 Iris 未启用**  
   - 查看 `ProjectSettings → Network → Enable Iris` 是否勾选（UE5.3+ 默认开启）。
3. **子对象未复制**  
   - 确认 `RegisterReplicationFragments` 被调用（可以在函数里加日志），以及外层组件（如 `UShootEquipmentManagerComponent`）是否把实例加入 `RegisteredSubObjectList`。

## 6. 参考资料
- 官方文档：《Unreal Engine Networking with Iris》（UE5 文档 / Networking）。
- Lyra 示例：`ULyraEquipmentInstance`、`ULyraInventoryItemInstance` 的注册模式。
- 本项目记录：`Docs/Engineering/DevelopmentLogs/2025-11/2025-11-14_库存系统会议记录.md`（2025-11-16 补丁条目）。

> TODO：后续落地更多 Iris 使用范例（如自定义 Fragment、聚合复制）时，再补充此文。
