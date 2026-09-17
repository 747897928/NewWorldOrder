// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "ShootHomeHubStateComponent.generated.h"

class UWorld;

/**
 * HomeMap 的轻量家园大厅状态入口。
 *
 * 调用链：BP_ShootGameState BeginPlay -> 在 HomeMap 的 Standalone 入口清理残留 CommonSession。
 * 菜单显示不属于本组件：玩家按下 IA_OpenMenu 后，由 BP_ShootCharacter.ShowMenuWidget 把
 * WBP_GameMenu 推入目标 LocalPlayer 的 CommonUI 层级。
 *
 * 这是 Lyra FrontendStateComponent 的项目化子集：只保留进入家园大厅时的残留会话清理边界，
 * 不迁入本项目目前不需要的 Press Start、Experience ControlFlow 和平台权限页面。HomeMap 暂时
 * 同时是家园大厅和联机目标图，因此 ListenServer/Client 加载该图时不能再次清理刚建立的会话。
 */
UCLASS(Blueprintable, ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootHomeHubStateComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	/** 仅当当前 World 与该地图一致时才启动 HomeHub 流程；在 BP_ShootGameState 配置为 HomeMap。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Home Hub")
	TSoftObjectPtr<UWorld> HomeHubMap;

private:
	bool IsConfiguredHomeHubWorld() const;
	void StartHomeHubFlow();
};
