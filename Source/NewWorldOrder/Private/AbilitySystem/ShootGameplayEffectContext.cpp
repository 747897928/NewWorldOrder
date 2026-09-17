// Copyright ZhaoYiJie

#include "AbilitySystem/ShootGameplayEffectContext.h"

#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Serialization/GameplayEffectContextNetSerializer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGameplayEffectContext)

FShootGameplayEffectContext* FShootGameplayEffectContext::ExtractEffectContext(FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseContext = Handle.Get();
	if (BaseContext && BaseContext->GetScriptStruct()->IsChildOf(FShootGameplayEffectContext::StaticStruct()))
	{
		return static_cast<FShootGameplayEffectContext*>(BaseContext);
	}
	return nullptr;
}

bool FShootGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// 爆炸半径只在服务器执行阶段使用；客户端不需要复制一次性伤害计算参数。
	return FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);
}

namespace UE::Net
{
	UE_NET_IMPLEMENT_FORWARDING_NETSERIALIZER_AND_REGISTRY_DELEGATES(
		ShootGameplayEffectContext, FGameplayEffectContextNetSerializer);
}
