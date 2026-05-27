# Operation Ultra

This is the current, consolidated Operation Ultra document. It replaces the split planning/log notes in `ultra_realistic.md` and `ultra_measurements.md`.

Operation Ultra is now a measured Fallout 4 performance effort for this DXVK branch, not a broad draw-call rewriting project.

## Goal

Improve Fallout 4 frame pacing and recover practical FPS in heavy scenes, especially Sim Settlements areas, without destabilizing DXVK's D3D11 draw submission path.

The current near-term target is modest and concrete:

- avoid the old 20 FPS regression class of experiments,
- keep normal DXVK draw submission intact,
- use the experimental SIMD build for contained CPU helper paths,
- use the D3D11 governor for controlled draw-budget experiments,
- measure stable vs experimental builds with the same save, camera angle, weather, and route,
- aim for practical gains such as the current 39 FPS settlement case moving toward 44 FPS.

## Current Reality

The current branch has three relevant performance tracks:

1. Regular clang build:

```text
build_clang\src\d3d11\d3d11.dll
build_clang\src\dxgi\dxgi.dll
```

2. Experimental SIMD clang build:

```text
build_clang_simd\src\d3d11\d3d11.dll
build_clang_simd\src\dxgi\dxgi.dll
```

3. D3D11 frame governor and draw budget controls:

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

The SIMD build is opt-in:

```powershell
$env:DXVK_BUILD_DIR='build_clang_simd'
$env:DXVK_EXPERIMENTAL_SIMD='1'
powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1
```

The regular build is:

```powershell
$env:DXVK_BUILD_DIR='build_clang'
$env:DXVK_EXPERIMENTAL_SIMD='0'
powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1
```

The experimental SIMD build was verified to include the current governor options because it is built from the same source tree.

## What Operation Ultra Is Not

The following ideas are not part of the current implementation plan:

- CPU-side per-draw frustum culling.
- CPU-side occlusion culling.
- Vertex-buffer bounds scanning per draw.
- Draw-count downscaling.
- Geometry rewriting.
- AVX2 gather-based draw batching.
- Replacing the core D3D11 draw path wholesale.
- Increasing `DirectMultiDrawBatchSize` without evidence.
- Reintroducing removed config options such as `d3d11.mergeDrawCalls`, `d3d11.frustumCull`, `d3d11.occlusionCull`, or `d3d11.downscaleFactor`.

Current source check:

- `DirectMultiDrawBatchSize` is still `256`.
- Old Ultra culling/downscale options are not active config options.
- D3D11 draw submission remains scalar.

## What Is Active

### Experimental SIMD

The SIMD work is documented in `AVX2.md`.

Current active direction:

- AVX2/FMA is applied to selected helper translation units.
- SIMD counters are compiled only when `DXVK_EXPERIMENTAL_SIMD=1`.
- The HUD item is `simd`.
- Default SIMD HUD graph and breakdown are off unless explicitly enabled.

Useful HUD line:

```ini
dxvk.hud = fps,drawcalls,simd,simd_graph=0,simd_breakdown=0
```

For debugging zones:

```ini
dxvk.hud = fps,drawcalls,simd,simd_graph=1,simd_breakdown=1
```

Current SIMD counter zones:

| Zone | Meaning |
| --- | --- |
| `Matrix` | Matrix/vector helper work. |
| `SpirvDec` | SPIR-V decompression. |
| `ImagePack` | CPU-side image packing/copy helpers. |
| `Descriptor` | Descriptor copy/clear/update helpers. |
| `Shader` | Shader key/hash/meta shader helper work. |
| `Pipeline` | Pipeline state hashing/equality/normalization. |
| `Memory` | Allocator page-mask helper work. |
| `Misc` | Stats, strings, LRU, implicit resolve helpers. |

### D3D11 SIMD State-Storm Early-Outs

The experimental SIMD build also has narrow D3D11 state-span equality helpers guarded by AVX2/x86_64 checks.

Current implemented helpers include:

- shader resource pointer span equality,
- sampler pointer span equality,
- vertex-buffer binding span equality,
- constant-buffer range span equality.

These are intended to reduce redundant state churn in Fallout 4-style repeated binding calls. They do not cull geometry, rewrite draw counts, or replace DXVK draw submission.

### D3D11 Governor

The governor is now the active draw-budget experiment.

It currently supports:

- frame pacing feedback from the presenter,
- early pacing sleep on the next frame,
- fixed draw budget through `d3d11.governorDrawBudget`,
- adaptive budget fallback through `d3d11.governorTargetPercent`,
- capped shedding through `d3d11.governorMaxSkipsPerFrame`,
- small-draw filtering through `d3d11.governorSkipMaxVertices`,
- conservative eligibility so shedding only targets small opaque depth-writing triangle draws.

The old `d3d11.drawPlateauMode` compatibility path still exists, but it should not be used for current Sim Settlements testing. It is too blunt and previously caused severe rendering starvation in turn-around tests.

## Current Sim Settlements Test Config

Use this first for the 39 FPS to 44 FPS attempt:

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

Tune in this order:

| Step | Draw Budget | Max Skips | Max Vertices | Purpose |
| --- | ---: | ---: | ---: | --- |
| 1 | 6500 | 500 | 24 | Safest first test. |
| 2 | 6000 | 700 | 24 | Stronger budget pressure. |
| 3 | 5500 | 900 | 32 | Strongest current test. |

Avoid this for the current test:

```ini
d3d11.drawPlateauMode = True
```

## Measurement Setup

Use the same save, camera angle, weather, time of day, route, resolution, mod list, and config for each comparison. Change one variable at a time.

| Field | Value |
| --- | --- |
| Date |  |
| GPU / Driver |  |
| CPU |  |
| Resolution |  |
| Fallout 4 preset / mods |  |
| ENB / ReShade |  |
| DXVK config file | `dxvk.conf` |
| Stable DLLs | `build_clang\src\d3d11\d3d11.dll`, `build_clang\src\dxgi\dxgi.dll` |
| SIMD DLLs | `build_clang_simd\src\d3d11\d3d11.dll`, `build_clang_simd\src\dxgi\dxgi.dll` |

Recommended HUD:

```ini
dxvk.hud = fps,drawcalls,simd,simd_graph=0,simd_breakdown=0
```

For governor-specific visibility, keep `drawcalls` enabled. The draw-call HUD reports:

- normal draw-call counts,
- skipped draw calls,
- governor early sleep and limiter sleep when active.

## Scene Results Log

Record at least 60 seconds per scene after the shader cache is warm, unless the test is specifically measuring cold-cache behavior.

| Scene | Build / Config | Avg FPS | 1% Low / Worst Observed | Draw Calls | Skipped Draws | Governor Sleep | SIMD us/frame | Notes |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Sim Settlements heavy view | Stable clang |  |  |  | n/a | n/a | n/a |  |
| Sim Settlements heavy view | SIMD only |  |  |  | n/a | n/a |  |  |
| Sim Settlements heavy view | SIMD + governor 6500/500/24 |  |  |  |  |  |  |  |
| Sim Settlements heavy view | SIMD + governor 6000/700/24 |  |  |  |  |  |  |  |
| Sim Settlements heavy view | SIMD + governor 5500/900/32 |  |  |  |  |  |  |  |
| Sanctuary Hills outdoor | Stable clang |  |  |  | n/a | n/a | n/a |  |
| Sanctuary Hills outdoor | SIMD |  |  |  | n/a | n/a |  |  |
| Diamond City market | Stable clang |  |  |  | n/a | n/a | n/a |  |
| Diamond City market | SIMD |  |  |  | n/a | n/a |  |  |
| Vault 111 indoor | Stable clang |  |  |  | n/a | n/a | n/a |  |
| Vault 111 indoor | SIMD |  |  |  | n/a | n/a |  |  |
| Combat / particles | Stable clang |  |  |  | n/a | n/a | n/a |  |
| Combat / particles | SIMD |  |  |  | n/a | n/a |  |  |
| Loading / transition | Stable clang |  |  |  | n/a | n/a | n/a |  |
| Loading / transition | SIMD |  |  |  | n/a | n/a |  |  |

## SIMD Zone Notes

Enable breakdown only when needed:

```ini
dxvk.hud = fps,drawcalls,simd,simd_graph=1,simd_breakdown=1
```

| Scene | Dominant SIMD Zone | Typical us/frame | Spike us/frame | Correlates With Stutter? | Notes |
| --- | --- | ---: | ---: | --- | --- |
| Sim Settlements heavy view |  |  |  |  |  |
| Sanctuary Hills outdoor |  |  |  |  |  |
| Diamond City market |  |  |  |  |  |
| Vault 111 indoor |  |  |  |  |  |
| Combat / particles |  |  |  |  |  |
| Loading / transition |  |  |  |  |  |

## Governor Notes

Use this table when tuning draw budget.

| Config | FPS Result | Governor Skip Visible? | Visual Issues? | Keep / Reject | Notes |
| --- | ---: | --- | --- | --- | --- |
| 6500 / 500 / 24 |  |  |  |  |  |
| 6000 / 700 / 24 |  |  |  |  |  |
| 5500 / 900 / 32 |  |  |  |  |  |

Interpretation:

- If `Governor skip` is `0`, the budget may not be crossed or the conservative eligibility filter may be rejecting all candidates.
- If FPS does not improve and `Governor skip` is active, the skipped draws may not be the bottleneck.
- If frames disappear or the scene only renders after turning around, the config is too aggressive or the old plateau path is active.
- If image quality breaks, raise the budget, lower max skips, or lower max vertices.

## Decision Log

| Change Tested | Result | Keep / Revert | Reason |
| --- | --- | --- | --- |
| Stable clang vs SIMD clang |  |  |  |
| SIMD clang + governor 6500/500/24 |  |  |  |
| SIMD clang + governor 6000/700/24 |  |  |  |
| SIMD clang + governor 5500/900/32 |  |  |  |

## Acceptance Criteria

A change is acceptable only if:

- regular clang build compiles,
- experimental SIMD clang build compiles,
- Fallout 4 launches and renders correctly,
- old Operation Ultra culling/downscale options are not reintroduced,
- draw counts are not rewritten as a fake performance win,
- no per-draw CPU resource scanning is added,
- the measured scene is equal or better in frame-time consistency,
- any visual quality loss is intentional, bounded, and documented.

Build verification:

```powershell
$env:DXVK_BUILD_DIR='build_clang'
$env:DXVK_EXPERIMENTAL_SIMD='0'
powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1

$env:DXVK_BUILD_DIR='build_clang_simd'
$env:DXVK_EXPERIMENTAL_SIMD='1'
powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1
```

Expected artifacts:

- `build_clang\src\d3d11\d3d11.dll`
- `build_clang\src\dxgi\dxgi.dll`
- `build_clang_simd\src\d3d11\d3d11.dll`
- `build_clang_simd\src\dxgi\dxgi.dll`

## Next Steps

1. Test the current experimental SIMD DLLs in the heavy Sim Settlements view.
2. Record stable, SIMD-only, and SIMD-plus-governor results in this document.
3. Watch `Governor skip`; if it stays at zero, add better governor eligibility/budget HUD counters next.
4. If the 6500 budget test is visually safe but not enough, try 6000/700/24.
5. If the scene blanks or starves, confirm `d3d11.drawPlateauMode` is off and back down the governor settings.
