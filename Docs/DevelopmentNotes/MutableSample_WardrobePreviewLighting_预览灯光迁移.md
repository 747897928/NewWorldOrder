# MutableSample WardrobePreviewLighting 预览灯光迁移

日期：2026-06-26
状态：[可用] [第一版迁移] [非最终视觉验收] [2026-08-14 增补：本文提到的 `UShootWardrobeWidgetBase::*` 函数已随 MVVM 迁移移除，对应预览相机逻辑现位于 `Source/NewWorldOrder/Private/UI/Wardrobe/ShootWardrobePreviewController.cpp`（`SetCameraMode` / `Zoom` / `ApplyCamera`）与 `AShootWardrobePreviewActor` 蓝图]

## 结论

MutableSample 的 Lobby 角色展示不是依赖场景太阳光，也不是用 UMG Viewport 默认大白光。它使用独立展示场景中的 4 盏低流明 `SpotLight` 包围角色，加 1 个指定 Cubemap 的 `SkyLight` 做环境光。

当前 NewWorldOrder 已把 MutableSample Lobby 的场景结构迁移到 `BP_ShootWardrobePreviewActor`，并关闭 `CharacterPreviewViewport` 默认灯光：

- `StudioBackdrop` 使用 `SM_SkySphere` 和衣柜自己的蓝灰渐变材质实例，避免矩形墙板边界进入画面。
- `StudioFloor` 使用圆柱地台和 glossy tile 材质。
- `StudioSkyLight` 使用 `GrayLightTextureCube`。
- `StudioPostProcess` 固定曝光并关闭 Bloom。
- 预览角色在 `W_Cloth` 中的 UI 遮挡、相机裁切、角色朝向仍需要截图验收。

## 来源

MutableSample MCP 9000，本轮复核：

- 当前关卡：`/Game/Lobby/Lobby`
- 请求与响应：
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_get_current_level.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_findactors_SpotLight.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_findactors_SkyLight.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_findactors_DirectionalLight.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_findactors_PostProcessVolume.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_findactors_CameraActor.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_getprops_SpotLight_7_LightComponent0.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_getprops_SpotLight_8_LightComponent0.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_getprops_SpotLight_9_LightComponent0.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_getprops_SpotLight_10_LightComponent0.response.json`
  - `Saved/CodexMCP/2026-06-26-wardrobe/mutablesample_9000_getprops_SkyLight_2_SkyLightComponent0.response.json`

本项目对照：

- `Source/NewWorldOrder/Private/UI/Wardrobe/ShootWardrobePreviewActor.cpp`
- `Source/NewWorldOrder/Public/UI/Wardrobe/ShootWardrobePreviewActor.h`
- `Source/NewWorldOrder/Private/UI/Wardrobe/ShootWardrobeWidgetBase.cpp`
- `/Game/UI/Mutable/BP_ShootWardrobePreviewActor`

## MutableSample Lobby 灯光数据

Lobby 中没有 `DirectionalLight`。这点很重要：衣柜预览不应该吃 HomeMap 的太阳光，也不应该吃 UViewport 默认白光。

### SpotLight_8

定位：主光 Key Light

- Actor：`/Game/Lobby/Lobby.Lobby:PersistentLevel.SpotLight_8`
- Location：`X=235.91, Y=92.54, Z=227.68`
- Rotation：`Pitch=-22.80, Yaw=-158.40, Roll=0`
- Intensity：`20`
- IntensityUnits：`Lumens`
- AttenuationRadius：`500`
- InnerConeAngle：`0`
- OuterConeAngle：`30`
- SourceRadius：`20`
- SoftSourceRadius：`10`
- UseInverseSquaredFalloff：`true`
- LightFalloffExponent：`8`
- Mobility：`Movable`
- CastDynamicShadows：`true`

### SpotLight_7

定位：补光 Fill Light

- Actor：`/Game/Lobby/Lobby.Lobby:PersistentLevel.SpotLight_7`
- Location：`X=-228.93, Y=160.32, Z=109.71`
- Rotation：`Pitch=3.00, Yaw=-42.00, Roll=0`
- Intensity：`7`
- IntensityUnits：`Lumens`
- AttenuationRadius：`600`
- InnerConeAngle：`0`
- OuterConeAngle：`28`
- SourceRadius：`30`
- SoftSourceRadius：`0`
- UseInverseSquaredFalloff：`true`
- LightFalloffExponent：`8`
- Mobility：`Movable`
- CastDynamicShadows：`true`

### SpotLight_9

定位：轮廓光 Rim Light

- Actor：`/Game/Lobby/Lobby.Lobby:PersistentLevel.SpotLight_9`
- Location：`X=-114.21, Y=-162.91, Z=247.18`
- Rotation：`Pitch=-32.00, Yaw=55.20, Roll=0`
- Intensity：`8`
- IntensityUnits：`Lumens`
- AttenuationRadius：`500`
- InnerConeAngle：`0`
- OuterConeAngle：`30`
- SourceRadius：`30`
- SoftSourceRadius：`0`
- UseInverseSquaredFalloff：`true`
- LightFalloffExponent：`8`
- Mobility：`Movable`
- CastDynamicShadows：`true`

### SpotLight_10

定位：背光 Back Light

- Actor：`/Game/Lobby/Lobby.Lobby:PersistentLevel.SpotLight_10`
- Location：`X=150.63, Y=-176.95, Z=195.38`
- Rotation：`Pitch=-16.00, Yaw=133.20, Roll=0`
- Intensity：`8`
- IntensityUnits：`Lumens`
- AttenuationRadius：`500`
- InnerConeAngle：`0`
- OuterConeAngle：`30`
- SourceRadius：`30`
- SoftSourceRadius：`0`
- UseInverseSquaredFalloff：`true`
- LightFalloffExponent：`8`
- Mobility：`Movable`
- CastDynamicShadows：`true`

### SkyLight_2

- Actor：`/Game/Lobby/Lobby.Lobby:PersistentLevel.SkyLight_2`
- Location：`X=-133, Y=-443, Z=-198`
- Component：`SkyLightComponent0`
- Intensity：`1`
- Mobility：`Stationary`
- SourceType：`SLS_SpecifiedCubemap`
- Cubemap：`/Game/Lobby/SceneElements/GrayLightTextureCube.GrayLightTextureCube`
- RealTimeCapture：`false`
- LowerHemisphereIsBlack：`true`
- LowerHemisphereColor：`R=0, G=0, B=0, A=1`
- CastDynamicShadows：`true`

### CameraActor

Lobby 中有两个 `CameraActor`：

- `CameraActor_0`：`Location X=-27.88, Y=161.92, Z=125.23`，`Rotation Pitch=0, Yaw=-90`
- `CameraActor_1`：`Location X=0, Y=363, Z=124`，`Rotation Pitch=-5, Yaw=-90`

当前衣柜没有直接使用这两个 CameraActor，而是用 `AShootWardrobePreviewActor::GetCameraView` 给 UViewport 设置相机位置。后续如果要更接近 MutableSample，应把这些相机点位移到 `BP_ShootWardrobePreviewActor`，并在发型、容貌等分类中使用平滑过渡。

### PostProcessVolume

Lobby 中存在两个 PostProcessVolume：

- `PostProcessVolume_0`
- `PostProcessVolume_3`

本轮只确认了 Actor 存在和位置，没有完整迁移 PostProcess 参数。当前衣柜的 `StudioPostProcess` 只是空的 `UPostProcessComponent`，还不是 MutableSample 的后处理效果。

## NewWorldOrder 当前迁移状态

`AShootWardrobePreviewActor` 当前已迁移：

- 4 个 `USpotLightComponent`：
  - `KeyLight`
  - `FillLight`
  - `RimLight`
  - `BackLight`
- 灯光单位改为 `Lumens`。
- 强度、衰减半径、锥角、SourceRadius 和 SoftSourceRadius 参考 MutableSample。
- `UShootWardrobeWidgetBase::InitializeCharacterPreview` 中已调用：
  - `CharacterPreviewViewport->SetLightIntensity(0.f)`
  - `CharacterPreviewViewport->SetSkyIntensity(0.f)`
  - 这样可以避免 UViewport 默认灯光把角色冲白。

当前尚未完整验收：

- 第二轮灯光是否已经消除半张脸过暗和白衣过曝。
- glossy tile 地台在 FullBody、Head、Footwear 镜头下的画面占比。
- 蓝灰 SkySphere 是否达到最终影棚美术要求。
- MutableSample 的 UI 隐藏后大面积角色欣赏模式。
- Mutable State 驱动的头部、发型、局部近景镜头。

## 当前架构判断

`/Game/UI/Mutable/BP_ShootWardrobePreviewActor` 已经存在。后续视觉调优应该优先落到这个蓝图子类，而不是继续在 C++ 构造函数里硬写更多视觉参数。

推荐职责：

- C++ `AShootWardrobePreviewActor`：
  - 负责创建组件。
  - 负责同步 PlayerState 性别和 Mutable 外观标签。
  - 负责 LeaderPose 同步和预览相机接口。
- 蓝图 `BP_ShootWardrobePreviewActor`：
  - 调整灯光位置、强度、颜色、阴影。
  - 配置 SkyLight Cubemap。
  - 分别配置 StudioFloor 与 StudioBackdrop 的网格和材质。
  - 配置 PostProcess。
  - 调整 FullBody、UpperBody、Head、Footwear 相机点位。

2026-07-10 已删除 C++ 构造函数中的舞台变换、相机点位、灯光数值和后处理默认值，也删除 `BeginPlay` 中的灯光朝向与动态材质覆盖。视觉默认值只保存在 `BP_ShootWardrobePreviewActor`。

## 2026-06-27 预览构图调参位置

用户验收发现角色仍被服装列表覆盖，根因不是灯光本身，而是相机仍按旧版居中构图拍角色。当前 `/Game/UI/Mutable/BP_ShootWardrobePreviewActor` 已按人物位于右侧预览区重新构图：

- `FullBodyCameraAnchor`：`RelativeLocation=(X=-20,Y=305,Z=88)`。
- `UpperBodyCameraAnchor`：`RelativeLocation=(X=-10,Y=205,Z=126)`。
- `HeadCameraAnchor`：`RelativeLocation=(X=0,Y=155,Z=151)`。
- `FootwearCameraAnchor`：`RelativeLocation=(X=-10,Y=190,Z=48)`。
- `PreviewFacingYawOffset=0`。

后续如果要微调，不要先改 `W_Cloth` 的格子或写死 C++ 旋转：

- 角色太大或太小：调对应相机锚点的 `RelativeLocation.Y`。
- 角色太靠中或被格子挡住：调对应相机锚点的 `RelativeRotation.Yaw`。往 `-90` 靠会更居中，往 `-40` 靠会更靠左。
- 角色只给玩家看侧影：调 `PreviewFacingYawOffset`，它定义在 `AShootWardrobePreviewActor`，蓝图子类 `/Game/UI/Mutable/BP_ShootWardrobePreviewActor` 可直接配置。
- 列表、页签、按钮遮挡角色：再检查 `W_Cloth` 的 `CharacterPreviewPanel`、`MainBorder`、`WardrobeSubTabsHBox` 和 `ClothUniformGrid` 层级与 Slot。

注意：本轮 MCP `CaptureEditorImage` 只能截到编辑器当前关卡视口，不能证明运行态衣柜最终效果。最终仍以 `HomeMap -> Play -> 打开 W_Cloth` 截图为准。

## 2026-07-04 预览缩放焦点规则

用户验收发现 `W_Cloth` 中 zoom in/out 时，角色不是围绕稳定中心放大，而是出现横向漂移。原因是旧实现只按 `CameraAnchor.Rotation.Vector()` 平移相机，缩放并不知道当前构图要围绕角色哪个点。

当前 C++ 已改为两点式相机，并在缩放时对焦点横向偏移做距离等比缩放：

- `CameraAnchor`：定义默认相机位置和分类切镜头的初始距离。
- `CameraFocusTarget`：定义画面中心和缩放稳定点。

相关属性定义在 `AShootWardrobePreviewActor`，蓝图子类 `/Game/UI/Mutable/BP_ShootWardrobePreviewActor` 可直接配置：

- `FullBodyCameraFocusTarget`
- `UpperBodyCameraFocusTarget`
- `HeadCameraFocusTarget`
- `FootwearCameraFocusTarget`

`UShootWardrobeWidgetBase::ApplyCharacterPreviewCamera` 会读取 `AShootWardrobePreviewActor::GetCameraView` 返回的 Anchor 与 FocusTarget，然后沿 `FocusTarget -> CameraAnchor` 方向做 dolly。为了避免 zoom in/out 时角色横向漂移，代码不会在缩放后继续看向原始 FocusTarget，而是以角色原点同高度位置作为 SubjectTarget，将 `FocusTarget - SubjectTarget` 这段横向构图偏移按 `当前相机距离 / 默认相机距离` 等比缩放，再让相机看向缩放后的 FocusTarget。

当前默认值只写在蓝图子类 `/Game/UI/Mutable/BP_ShootWardrobePreviewActor`：

- `FullBodyCameraFocusTarget=(X=-190,Y=0,Z=88)`
- `UpperBodyCameraFocusTarget=(X=-145,Y=0,Z=126)`
- `HeadCameraFocusTarget=(X=-105,Y=0,Z=151)`
- `FootwearCameraFocusTarget=(X=-140,Y=0,Z=48)`

调参建议：

- 角色需要整体更靠左：在对应模式下把 FocusTarget 的 X 调大一点，或微调 CameraAnchor 的 Yaw。
- 角色缩放时横向漂移：优先检查 `UShootWardrobeWidgetBase::ApplyCharacterPreviewCamera` 是否仍在使用距离等比缩放后的 FocusTarget，不要退回到只沿相机 Forward 平移的旧实现。
- 角色太大或太小：优先调 CameraAnchor 与 FocusTarget 的距离，或调 CameraAnchor 的 `RelativeLocation.Y`。
- 切换分类后 zoom 应回默认：由 `UShootWardrobeWidgetBase::ResetCharacterPreviewZoomForModeChange` 负责，分类切换不要保留上一分类的 zoom 偏移。

这仍是第一版构图修正。最终是否合格必须以 `HomeMap -> Play -> 打开 W_Cloth` 的真实截图和手感为准。

## 下一步建议

1. 关闭 UnrealEditor 后重新执行完整 C++ 构建，让删除的运行时视觉覆盖真正进入编辑器二进制。
2. 对 `W_Cloth` 做新的 PIE 截图验收：
   - 预览角色不应过曝。
   - UI 不应挡住角色主体。
   - 服装列表应在右侧内容区，不应压到角色脸和身体中心。
   - 关闭或隐藏 UI 后应能欣赏全身角色。

## 当前验收声明

本笔记只说明第一版迁移依据。它不能替代视觉验收。

最终是否合格必须以 `HomeMap -> Play -> 打开 W_Cloth -> 真实截图` 为准。如果截图仍然出现白屏、曝光过高、角色裁切、UI 覆盖角色主体，则说明迁移还没有完成。
