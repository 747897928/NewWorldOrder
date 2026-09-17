# UE 5.8 macOS Apple Silicon: packaged game permanently freezes in Metal present after runtime character-generation updates

## Summary

A packaged UE 5.8 ARM64 game can permanently stop rendering on an Apple M1 Pro after a runtime `CustomizableObject` / Mutable skeletal-mesh update. The same terminal state is reproducible through two independent paths:

1. Opening an offline character-customization screen that renders a Mutable character in a `UViewport` preview world.
2. Creating a Steam lobby and travelling to the listen-server map; the Steam workflow completes successfully, then the gameplay character performs its normal runtime Mutable update.

The process does not crash. GameThread stops at the frame-end render fence while the RHI submission thread waits in Metal present. The same project workflow runs on Windows.

## Environment

| Item | Value |
| --- | --- |
| Engine | Unreal Engine 5.8.0 Launcher build `55116800.0.8` |
| Build | Packaged macOS ARM64 Development and Shipping |
| Hardware | Apple M1 Pro |
| OS | macOS 26.5.2 (25F84) |
| Renderer | Metal, SM5 |
| Mutable | Bundled UE 5.8 Mutable plugin 1.8.0 |
| Control | Same project behavior works on Windows |

## Expected result

The game continues to render and tick after a runtime character-generation update, whether it is initiated by an offline customization preview or by a normal listen-server map transition.

## Actual result

The visible frame remains on screen permanently. HUD/input/tick no longer progress because GameThread is blocked waiting for rendering. In some configurations the app can still be terminated normally; in others the macOS UI becomes unresponsive.

## Reproduction A — offline, no Steam or networking

1. Launch the packaged game normally.
2. Enter the gameplay map. The gameplay character's first runtime Mutable output is visible.
3. Open the character-customization UI. It creates a preview character in a `UViewport` preview world and submits one asynchronous Mutable skeletal-mesh update for that preview.
4. The game may freeze while the preview is being created or immediately after the update reports completion.

No Session is created, found, joined, or hosted in this reproduction.

## Reproduction B — normal Steam host

1. Launch the packaged game through Steam.
2. Create one Steam session/lobby.
3. The log confirms, in order: successful lobby creation, successful listen-server map travel, `SteamSocketsNetDriver` listening on port 7777, and successful session start.
4. The gameplay character begins its normal Mutable skeletal-mesh update after the map transition.
5. The game freezes in the same rendering state.

This shows Steam session creation and network travel are trigger paths, not the blocking call stack: `OnlineAsyncTaskThreadSteam` remains active in `SteamAPI_RunCallbacks` while GameThread is blocked on the render fence.

## Consistent process samples

All samples were taken while the frame was frozen. Their common terminal state is:

```text
GameThread
  FEngineLoop::Tick
  FFrameEndSync::Sync
  FRenderCommandFence::Wait

RHISubmissionThread
  FMetalRHICommandContext::EndDrawingViewport
  FMetalViewport::Present
  FMetalViewport::PresentDrawLayers
  dispatch_semaphore_wait

Metal command-queue dispatch
  IOGPUMetalCommandQueue submitCommandBuffers
```

This indicates the command-buffer completion/present wait does not return. A post-freeze sample cannot prove which prior draw, resource, or plugin operation caused the command buffer not to complete.

## What has been tested

- `r.AllowOcclusionQueries=False`: did not prevent the present wait.
- `r.RHICmdBypass=1`: did not prevent the present wait.
- `-norhithread`: changed responsiveness/exit behavior but did not prevent frozen rendering.
- `-gpulockstep`: made the game unusably slow before the reproduction point; not viable.
- `Mutable.MaxTextureSizeToGenerate=1024`: did not prevent the freeze and was removed.
- Development run with `-MetalRuntimeDebugLevel=2`, `LogMutable VeryVerbose`, `LogMetal VeryVerbose`, and `LogRHI Verbose`: changes timing and creates substantial expected overhead, but does not prevent the same freeze; no Metal validation error was logged before it.

No Unreal Engine, Mutable plugin, or project C++ source was modified for these tests.

## macOS diagnostics from the GPU-capture attempt

An Xcode GPU Frame Capture of a normal frame was attempted. The resulting trace was valid, but attempting to capture the reproduction caused a full macOS UI stall and reboot was required. macOS generated a WindowServer watchdog report showing WindowServer unresponsive for 40 seconds. The associated stack includes:

```text
GPUToolsReplayService
AGXMetalG13X
IOGPUMetalCommandQueue submitCommandBuffers
IOGPUCommandQueueSubmitCommandBuffers
```

This does not independently prove a driver defect or identify the project resource, but it prevents safe repeated GPU-capture attempts on the affected machine.

## Requested guidance

1. Is this a known UE 5.8 / macOS 26.5 / Apple M1 Pro Metal issue involving `FMetalViewport::PresentDrawLayers` command-buffer completion waits?
2. Is the use of runtime Mutable skeletal-mesh updates in a packaged macOS game, including an independent `UViewport` preview-world instance, supported on this configuration?
3. Is there an Epic fix, hotfix, later engine build, or recommended engine-side diagnostic for identifying the command buffer that never completes?
4. What minimal project-level workaround, if any, is recommended without disabling runtime Mutable character generation?

## Attachments

Attach the following files from `Build/Mac`:

- `Steam-host-freeze.sample.txt` — normal Steam host reproduction.
- `Wardrobe-open-freeze-metaldebug.sample.txt` — offline wardrobe reproduction under Metal validation.
- `Wardrobe-switch-freeze.sample.txt` — another offline wardrobe sample after character-switch requests.
- `Wardrobe-Metal.log` — Development diagnostic log.
- `NewWorldOrder.gputrace` — normal one-frame GPU capture; provide on request because it is 139 MB and does not contain the freeze.

Also attach, if the report portal accepts system diagnostics:

- `/Library/Logs/DiagnosticReports/WindowServer-2026-07-24-121803.ips`
- `/Library/Logs/DiagnosticReports/WindowServer_2026-07-24-121844_zhaoyijiedeMacBook-Pro.userspace_watchdog_timeout.spin`
