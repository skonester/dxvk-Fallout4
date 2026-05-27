# Governor Implementation State

This is the consolidated current implementation state for the Fallout 4 DXVK D3D11 governor. It replaces the split planning notes in `implementation.md` and `implementationplan.md`.

The old docs were useful for the landing path, but parts of them are now historical. This file reflects the current source tree.

## Problem

`dxgi.maxFrameRate` limits presentation pacing, but the limiter traditionally sleeps after the D3D11 frame has already submitted its work. That can cap output, but it cannot stop Fallout 4 from recording an expensive frame before the wait happens.

DXVK also cannot make Fallout 4 avoid issuing D3D11 calls before those calls reach DXVK. The real levers DXVK has are:

1. Move limiter back-pressure earlier so the next frame does not race ahead as hard.
2. Optionally skip selected GPU draw commands after DXVK receives them.

The first lever is safe. The second lever is a visual tradeoff and must stay opt-in.

## Current Goal

The current target is the heavy Sim Settlements case where FPS drops from about 60 to 39 while looking at the settlement. The near-term goal is to recover roughly 5 FPS, moving the bad view toward 44 FPS, while avoiding the previous failure mode where the scene stopped rendering until the camera turned away.

## Implemented State

### Limiter Sleep Reporting

Implemented.

Files:

- `src/util/util_fps_limiter.h`
- `src/util/util_fps_limiter.cpp`
- `src/dxvk/dxvk_presenter.cpp`

Current behavior:

- `FpsLimiter::delayWithStats()` returns the limiter sleep duration.
- Existing `delay()` behavior is preserved by calling the stats-capable path.
- Presenter code records limiter sleep in both relevant paths:
  - non-present-wait path in `Presenter::signalFrame`,
  - present-wait frame thread path in `Presenter::runFrameThread`.
- Limiter sleep is reported to the frame governor when attached.
- Limiter sleep is accumulated in `DxvkStatCounter::GovernorLimiterSleepUs`.

### Frame Governor Core

Implemented.

Files:

- `src/util/util_frame_governor.h`
- `src/util/util_frame_governor.cpp`
- `src/util/meson.build`

Current types:

```cpp
FrameGovernorMode
FrameGovernorOptions
FrameGovernorStats
FrameGovernor
```

Current modes:

```text
Disabled
Observe
Pace
Shed
Recover
```

Current options:

```cpp
bool enabled
bool pacing
bool drawShedding
uint32_t drawBudget
uint32_t targetPercent
uint32_t skipMaxVertices
uint32_t recoveryFrames
```

Current behavior:

- Tracks target frame interval from the swapchain frame-rate limit.
- Accepts limiter sleep feedback from the presenter.
- Converts part of a post-present limiter sleep into a pending early delay.
- Clamps early delay to half of the target frame interval.
- Tracks per-frame direct draw count, skipped draw count, draw budget, limiter sleep, and early sleep.
- Supports fixed draw budget through `drawBudget`.
- Falls back to adaptive budget through `targetPercent` when fixed budget is zero.
- Allows draw shedding only when enabled, over budget, not recovering, and under the configured vertex limit.

Current limitation:

- `recoveryFrames` exists in options/state, but there is not yet a robust trigger for scene transitions, loading, resize, or UI-heavy recovery. The recovery counter currently only counts down if set.

### D3D11 Options

Implemented.

Files:

- `src/d3d11/d3d11_options.h`
- `src/d3d11/d3d11_options.cpp`
- `conf/dxvk.conf`
- `conf/sim_settlements_governor_budget_test.conf`

Current governor options:

```ini
d3d11.governorMode
d3d11.governorPacing
d3d11.governorDrawShedding
d3d11.governorDrawBudget
d3d11.governorMaxSkipsPerFrame
d3d11.governorTargetPercent
d3d11.governorSkipMaxVertices
d3d11.governorRecoveryFrames
```

Current defaults:

```ini
d3d11.governorMode = False
d3d11.governorPacing = True
d3d11.governorDrawShedding = False
d3d11.governorDrawBudget = 0
d3d11.governorMaxSkipsPerFrame = 500
d3d11.governorTargetPercent = 110
d3d11.governorSkipMaxVertices = 64
d3d11.governorRecoveryFrames = 30
```

Compatibility options still exist:

```ini
d3d11.drawPlateauMode
d3d11.drawPlateauBaseline
d3d11.drawPlateauTargetPercent
d3d11.drawPlateauSkipMaxVertices
```

Current typo aliases:

```ini
d3d11.govenorDrawBudget
d3d11,govenorDrawBudget
d3d11,governorDrawBudget
d3d11.govenorMaxSkipsPerFrame
```

Known gap:

- Lowercase typo variants such as `d3d11,govenordrawbudget` are not currently parsed.

### Governor Ownership And Wiring

Implemented.

Files:

- `src/d3d11/d3d11_swapchain.h`
- `src/d3d11/d3d11_swapchain.cpp`
- `src/dxvk/dxvk_presenter.h`
- `src/dxvk/dxvk_presenter.cpp`

Current ownership:

- `D3D11SwapChain` owns `FrameGovernor m_governor`.
- The presenter stores a raw optional `FrameGovernor*`.
- `D3D11SwapChain` clears the presenter pointer before destroying presenter resources.

Current swapchain wiring:

- Constructor maps D3D11 options into `FrameGovernorOptions`.
- `SetTargetFrameRate` calls `m_governor.setTargetFrameRate(FrameRate)`.
- `CreatePresenter` calls `m_presenter->setFrameGovernor(&m_governor)`.
- Destructor path clears the pointer with `m_presenter->setFrameGovernor(nullptr)`.

### Early Pacing

Implemented.

File:

- `src/d3d11/d3d11_swapchain.cpp`

Current behavior in `D3D11SwapChain::PresentImage`:

1. Calculates `nextFrameId = m_frameId + 1`.
2. Calls `m_governor.beginFrame(nextFrameId)`.
3. Calls `m_governor.getEarlyPaceDelay(nextFrameId)`.
4. If nonzero, sleeps before taking the immediate context lock.
5. Adds `DxvkStatCounter::GovernorEarlySleepUs`.

This keeps the sleep out of the immediate context lock and moves part of limiter wait earlier in the next frame.

### Draw Counting And Shedding

Implemented for direct draws.

Files:

- `src/d3d11/d3d11_context.h`
- `src/d3d11/d3d11_context.cpp`
- `src/d3d11/d3d11_context_imm.cpp`

Current behavior:

- Direct draw entry points call `ShouldSkipDrawForPlateau`.
- `m_drawPlateauCount` counts direct draw calls for the current frame.
- `m_drawPlateauSkipped` counts skipped direct draw calls for the current frame.
- `EndDrawGovernorFrame(FrameGovernor* Governor, uint64_t FrameId)` reports counts to the governor and resets the per-frame counters.
- The old reset from `D3D11ImmediateContext::EndFrame` was removed so counts survive until the swapchain reports the frame boundary.

Current direct draw coverage:

- `Draw`
- `DrawIndexed`
- `DrawInstanced`
- `DrawIndexedInstanced`

Current non-coverage:

- Indirect draws are not shed.
- Compute dispatches are not shed.
- Clears, uploads, barriers, resource maps, and engine-side CPU work are not shed.

### Safer Draw Eligibility

Implemented as a conservative first pass.

File:

- `src/d3d11/d3d11_context.cpp`

Current `IsDrawGovernorEligible()` requires:

- topology is `D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST` or `D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP`,
- a depth-stencil view is bound,
- depth is enabled when depth-stencil state exists,
- depth writes are enabled when depth-stencil state exists,
- stencil is not enabled,
- alpha-to-coverage is not enabled,
- render-target blend is not enabled,
- logic op is not enabled.

This intentionally rejects many draws. False negatives are acceptable for now; false positives can remove important visuals.

Current safety cap:

- `d3d11.governorMaxSkipsPerFrame` caps how many eligible draws can be skipped per frame.

### HUD And Stats

Partially implemented.

Files:

- `src/dxvk/dxvk_stats.h`
- `src/dxvk/hud/dxvk_hud_item.h`
- `src/dxvk/hud/dxvk_hud_item.cpp`

Current counters:

```cpp
CmdDrawCallsSkipped
GovernorLimiterSleepUs
GovernorEarlySleepUs
```

Current HUD behavior:

- Draw-call HUD shows skipped draw calls as `Governor skip`.
- Draw-call HUD shows early/limiter sleep as `Governor sleep`.

Not implemented yet:

- HUD-visible governor mode.
- HUD-visible governor draw budget.
- HUD-visible eligible draw count.
- HUD-visible over-budget candidate count.

Those missing counters matter because if `Governor skip` stays at zero, we need to know whether the budget was not crossed or the eligibility filter rejected all candidates.

## Current Test Config

Use the dedicated config file:

```text
conf\sim_settlements_governor_budget_test.conf
```

Current first test:

```ini
[Fallout4.exe]
dxgi.maxFrameRate = 60
dxgi.maxFrameLatency = 1
dxgi.syncInterval = 1

d3d11.governorMode = True
d3d11.governorPacing = True
d3d11.governorDrawShedding = True
d3d11.governorDrawBudget = 6500
d3d11.governorMaxSkipsPerFrame = 500
d3d11.governorSkipMaxVertices = 24
d3d11.governorTargetPercent = 110
d3d11.governorRecoveryFrames = 30

d3d11.cachedDynamicResources = a
d3d11.relaxedBarriers = True
d3d11.maxTessFactor = 8
d3d11.samplerLodBias = 0.0

dxvk.useRawSsbo = True
dxvk.numCompilerThreads = 0

dxvk.hud = fps,drawcalls,simd,simd_graph=0,simd_breakdown=0
```

Tune order:

| Step | Budget | Max Skips | Max Vertices |
| --- | ---: | ---: | ---: |
| Safer | 6500 | 500 | 24 |
| Stronger | 6000 | 700 | 24 |
| Strongest current test | 5500 | 900 | 32 |

Do not use this for the current test:

```ini
d3d11.drawPlateauMode = True
```

That compatibility path is older and too blunt for the Sim Settlements test.

## Build State

The user specifically wants `build_dx11.ps1` used.

Regular clang build:

```powershell
$env:DXVK_BUILD_DIR='build_clang'
$env:DXVK_EXPERIMENTAL_SIMD='0'
powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1
```

Experimental SIMD clang build:

```powershell
$env:DXVK_BUILD_DIR='build_clang_simd'
$env:DXVK_EXPERIMENTAL_SIMD='1'
powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1
```

Current verified SIMD artifacts:

```text
build_clang_simd\src\d3d11\d3d11.dll
build_clang_simd\src\dxgi\dxgi.dll
```

The experimental SIMD build has been verified to contain the current governor option strings.

## Validation Checklist

### Pacing

- With `d3d11.governorPacing = True`, HUD should show `Governor sleep` when limiter sleep is happening.
- Above-limit scenes should feel paced rather than frozen after a fully queued frame.
- Below-limit scenes should not gain meaningful early sleep if the limiter is not sleeping.

### Shedding

- With `d3d11.governorDrawShedding = False`, `Governor skip` should remain zero.
- With shedding enabled and budget crossed, `Governor skip` should become nonzero.
- HUD/menu elements should not disappear.
- If the scene stops rendering until the camera turns away, settings are too aggressive or `drawPlateauMode` is active.

### Sim Settlements

- Test the exact same camera view for stable, SIMD-only, and SIMD plus governor.
- Record FPS, draw calls, skipped draws, and governor sleep.
- Target is roughly 39 FPS moving toward 44 FPS without severe artifacts.

## Known Limitations

- The governor cannot prevent Fallout 4 from issuing D3D11 calls before DXVK receives them.
- Draw shedding only affects selected direct draws after DXVK sees them.
- Eligibility is intentionally strict and may skip nothing in some scenes.
- There is no HUD counter yet for eligible candidates or over-budget rejected draws.
- Recovery mode lacks robust triggers for loading, alt-tab, resize, or UI transitions.
- The legacy draw plateau compatibility path still exists and can be harmful if enabled.
- Lowercase typo aliases for `govenordrawbudget` are not parsed yet.

## Next Implementation Steps

1. Add HUD/stat counters for:
   - governor draw budget,
   - eligible draw count,
   - over-budget eligible candidate count,
   - governor mode.
2. Add lowercase typo aliases for:
   - `d3d11.govenordrawbudget`,
   - `d3d11,govenordrawbudget`,
   - `d3d11.govenormaxskipsperframe`.
3. Add recovery triggers for resize, occlusion, loading-like frame gaps, and large timing discontinuities.
4. If `Governor skip` stays zero in the settlement test, decide whether to loosen eligibility or lower budget based on the new counters.
5. If skipping works but FPS does not improve, the skipped draws are not the bottleneck; shift effort to diagnostics rather than increasing visual damage.

## Practical Truth Table

| Condition | Expected Governor Behavior |
| --- | --- |
| FPS above cap and limiter sleeps | Move part of wait earlier next frame |
| FPS below cap and no limiter sleep | Do not pace |
| Draw shedding off | Do not skip draws |
| Draw shedding on, over budget, eligible tiny draw | Skip within max-skips cap |
| UI/menu/transparent/stencil/no-depth draw | Do not skip |
| Loading/alt-tab/resize | Should recover; robust triggers still need work |

## Bottom Line

The governor is no longer just a plan. The current tree has limiter sleep feedback, a frame governor object, early pacing, direct draw counting, conservative draw shedding, fixed draw budgets, max-skip caps, and HUD visibility for sleep/skip counts.

The next real bottleneck is observability: we need to know whether the settlement scene has eligible over-budget draws. Add those counters before making shedding more aggressive.
