# Draft for Epic: macOS Metal freeze reproducible through wardrobe Mutable preview and Session/Join paths

Status: draft only. This is not an external submission and does not claim a confirmed root cause.

## Title

UE 5.8 macOS: packaged game can freeze in Metal present; reproducible without networking by opening a `UViewport` wardrobe preview that creates a second Mutable (`CustomizableObject`) instance

## Environment

- Unreal Engine: 5.8.0, Launcher build `55116800.0.8`.
- Mutable plugin: bundled UE 5.8 plugin, version `1.8.0`.
- Hardware: Apple Silicon M1 Pro.
- OS: macOS 26.5.2 (25F84).
- Build: packaged ARM64 Shipping game, not the editor.
- Control platform: the same project workflow works on Windows.

## Minimal reproduction in the affected project

1. Launch the packaged macOS game normally. Do not create, find, or join any Online Session.
2. Enter the game world. The player character has already completed its normal Mutable generation successfully.
3. Open the wardrobe widget (`/Game/UI/Mutable/W_Cloth`).
4. The widget creates a `UViewport` preview actor (`/Game/UI/Mutable/BP_ShootWardrobePreviewActor`) in its preview world.
5. The preview actor creates a new `UCustomizableObjectInstance` for the active gender, attaches that instance to two `UCustomizableSkeletalComponent`s (Head and Body), then calls `UpdateSkeletalMeshAsync()` once with the player's current appearance tags.
6. The game frame freezes permanently. It can be terminated normally; this is not an application crash.

Expected: the wardrobe preview creates and displays its Mutable character without freezing.

Actual: rendering stops immediately after the wardrobe is opened, without any Session or networking activity.

## Scope clarification

The same project previously reproduced the same Metal present wait during Windows Host -> macOS Client Join, when the wardrobe was not opened. Therefore the preview actor is **not** a necessary condition for every freeze and is not claimed as the root cause. The wardrobe path is valuable because it creates a reliable, offline reproduction with the same observed wait, eliminating Steam, Session management, and network travel as necessary conditions for that reproduction.

## Observed sample

`sample` captured after the freeze at 2026-07-24 10:41 +0800, PID `21886`:

```text
GameThread
  FEngineLoop::Tick
  FFrameEndSync::Sync
  FRenderCommandFence::Wait

RHIThread
  FMetalRHICommandContext::EndDrawingViewport
  FMetalViewport::Present
  FMetalViewport::PresentDrawLayers
  dispatch_semaphore_wait
```

The Metal command-queue dispatch thread is blocked in `IOGPUMetalCommandQueue` submission. The process physical footprint at sampling was about 2.7 GB. The sample does not show a Mutable function on stack because it was taken after the render pipeline had already stopped; it demonstrates a Metal command-buffer completion/present wait, not a proven fault inside Mutable.

Development-package reproduction at 2026-07-24 11:25 +0800 produced the same wait. Its runtime log records the gameplay Mutable update at frame 313, then, on opening the wardrobe, a second `LogMutable: Started Update Skeletal Mesh Async` for a different `CustomizableObjectInstance` at frame 513. The wardrobe UI finishes its focus update in that frame; frame 514 logs the preview LUT format, and no subsequent game frame is logged. The accompanying process sample again shows the GameThread render-fence wait, RHI submission in `PresentDrawLayers`, and the Metal queue in `IOGPUMetalCommandQueue` submission.

This particular Development run was launched from Finder (`Command Line: -installed`), so it did **not** include `-MetalRuntimeDebugLevel=2` or the requested verbose `LogCmds`; that targeted validation/logging run remains outstanding.

## Targeted Metal-validation reproduction

The current Development package was later launched directly with `-MetalRuntimeDebugLevel=2 -LogCmds="LogMutable VeryVerbose,LogMetal VeryVerbose,LogRHI Verbose"`. This makes rendering substantially slower because the validation layer and first-use pipeline-state creation are active, but the initial wardrobe preview completed successfully:

- `CO_Character_M` preview update completed in about 1.10 s and the preview was displayed.
- Changing character gender then reproduced the permanent freeze. A fresh 10 s sample (`Wardrobe-switch-freeze.sample.txt`, 2026-07-24 11:34 +0800) again shows `GameThread -> FFrameEndSync::Sync -> FRenderCommandFence::Wait`, `RHISubmissionThread -> FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait`, and Metal's command-queue dispatch in `IOGPUMetalCommandQueue` submission.
- No Metal validation error, command-buffer error, or Mutable error was emitted before the stall.

The verbose log shows three explicit `SwitchToCharacter` transitions in approximately 1.3 seconds (`male -> female`, `female -> male`, then `male -> female`). The user confirmed these were separate clicks made because the UI had become unresponsive; they are not evidence that the project spontaneously re-entered the gender-switch operation. For each transition the project intentionally submits one update for the gameplay character COI and one update for the wardrobe-preview COI. This shows that the failure can be reached after valid Mutable updates have completed while additional gender/output work is pending, but it does not establish a Mutable-internal fault.

On a subsequent clean launch using the same validation command, the game again froze on the *first* wardrobe open before the switch button could be used. The preview COI had started and completed the male update (`UpdateTime=0.047291 s`), then the frame stopped. A separate 10 s sample (`Wardrobe-open-freeze-metaldebug.sample.txt`, 2026-07-24 11:40 +0800) has the same GameThread render-fence wait, `PresentDrawLayers` semaphore wait, and `IOGPUMetalCommandQueue` submission. Therefore validation changes the timing/probability only; neither gender switching nor repeated user input is a necessary condition.

## Steam host cross-check

When launched normally through Steam (not via the diagnostic terminal command), creating one Steam session also froze after HomeMap appeared. The runtime log establishes that Steam itself completed the host workflow before the rendering stall:

1. `OnCreateSessionComplete(... bWasSuccessful: 1)` with a real Steam lobby id.
2. Server travel to `/Game/Maps/HomeMap?listen` completed; `SteamSocketsNetDriver` was listening on port 7777.
3. `OnStartSessionComplete(... bWasSuccessful: 1)` and the HomeMap listen-server state were logged.
4. The player Mutable instance then logged `Started Update Skeletal Mesh Async`; the next render frame stalled.

The 10 s `Steam-host-freeze.sample.txt` (2026-07-24 11:46 +0800) has the same Metal present/completion wait as the offline wardrobe samples. `OnlineAsyncTaskThreadSteam` continues in `SteamAPI_RunCallbacks`; it is not blocking the GameThread. This confirms that Session creation/network travel is a second trigger path, while the shared terminal failure remains the Metal command-buffer/present completion wait.

## Xcode GPU capture attempt and system watchdog

An Xcode GPU Frame Capture attempt was made against the deployed Development app with the scheme's GPU Frame Capture option set to Metal. The capture did not produce a recoverable `.gputrace`; instead, macOS became unresponsive and required a reboot. This must not be repeated as a routine reproduction step.

After reboot, macOS generated system diagnostics at 2026-07-24 12:18 +0800:

- `WindowServer-2026-07-24-121803.ips` reports `WATCHDOG: monitoring timed out for service`, with WindowServer unresponsive for 40 seconds.
- The watchdog stack includes `GPUToolsReplayService`, `AGXMetalG13X`, `IOGPUMetalCommandQueue submitCommandBuffers`, and `IOGPUCommandQueueSubmitCommandBuffers` while replaying a captured render encoder.
- `WindowServer_2026-07-24-121844_...userspace_watchdog_timeout.spin` and the Xcode CPU resource diagnostic were generated at the same time.

This is evidence that the capture/replay path reached an Apple GPU/Metal queue stall. It does not independently identify the project resource or prove a driver defect, but it increases the severity of the report and makes a source-level or engine-vendor investigation preferable to repeated local GPU captures.

## Project lifecycle details

- The gameplay pawn and the wardrobe preview use separate `UCustomizableObjectInstance`s for the same `UCustomizableObject`.
- The preview uses one instance for two components, Head and Body; it does not intentionally request two updates for its first display.
- The first preview update is submitted by `AShootWardrobePreviewActor::InitializePreviewIfNeeded -> UMutableAppearanceComponent::InitializePreviewComponents -> ApplyPreviewAppearanceTags -> UCustomizableObjectInstance::UpdateSkeletalMeshAsync`.
- `UCustomizableObjectInstance::UpdateSkeletalMeshAsync` queues work in `UCustomizableObjectSystem`; it generates mesh/material/image resources asynchronously and applies the result back to components.

## Attempts and results

- `r.AllowOcclusionQueries=False` on macOS: did not prevent the later Metal present wait.
- `r.RHICmdBypass=1`: did not prevent the same `PresentDrawLayers` wait.
- `-norhithread`: UI progressed further but the rendered frame still froze.
- `-gpulockstep`: made offline gameplay unusably slow before opening a Session, so it is not a viable workaround.
- `Mutable.MaxTextureSizeToGenerate=1024` on macOS only: did not prevent the freeze. The wardrobe UI and base preview pawn were visible before the frame stopped; a sample captured at 2026-07-24 11:01 +0800 showed the same `FFrameEndSync::Sync -> FRenderCommandFence::Wait` and `FMetalViewport::PresentDrawLayers -> dispatch_semaphore_wait` chain. The setting has been removed; texture-size capping is not a workaround.

## Requested guidance

1. Is this a known UE 5.8 / macOS 26 / Apple Silicon issue involving Mutable-generated resources and `FMetalViewport::PresentDrawLayers`?
2. Is creating a second runtime Mutable instance in a `UViewport` preview world a supported path on macOS in UE 5.8?
3. Which Metal/Mutable diagnostics should be collected to identify the command buffer that never completes, without modifying Engine or plugin source?
4. Are there known fixes or later engine builds that address this completion-handler semaphore wait?

## Attachments to include with an Epic report

- `NewWorldOrder-host-freeze.sample.txt` (the no-session wardrobe freeze sample).
- Project `Saved/Logs` from the same run, with `LogMutable` and `LogMetal` verbosity enabled if possible.
- A minimal reproduction project or the two CustomizableObject assets and preview actor blueprint, if Epic requests them.
- The exact packaging log and a System Information report showing the macOS build and M1 Pro GPU.
