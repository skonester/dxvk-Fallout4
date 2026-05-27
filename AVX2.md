# AVX2 Experimental SIMD Status

This document is the single current reference for AVX2/FMA work in this Fallout 4 DXVK branch. It combines the former `avx2hotpath.md` and `avx2stats.md` notes, plus the current SIMD counter implementation from the source tree.

`simcounter.md` is not present in this checkout, so the counter section below is based on the implemented files:

- `src/util/util_simd_perf.h`
- `src/util/util_simd_perf.cpp`
- `src/dxvk/hud/dxvk_hud.cpp`
- `src/dxvk/hud/dxvk_hud_item.cpp`

## Current Reality

The AVX2 path is an opt-in experimental build, not the default build.

Build it with:

```powershell
$env:DXVK_BUILD_DIR='build_clang_simd'
$env:DXVK_EXPERIMENTAL_SIMD='1'
powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1
```

The current SIMD build directory is:

```text
build_clang_simd
```

The expected test DLLs are:

```text
build_clang_simd\src\d3d11\d3d11.dll
build_clang_simd\src\dxgi\dxgi.dll
```

The Meson option is:

```text
dxvk_experimental_simd=true
```

When enabled, Meson adds AVX2/FMA compiler flags and sets:

```text
DXVK_SIMD_PERF=1
```

That means the SIMD HUD item and SIMD cycle counters are compiled into the experimental build. In a normal non-SIMD build, `DXVK_SIMD_PERF` is `0`, and SIMD HUD/counter code is compiled out.

## Build Wiring

The build option is declared in `meson_options.txt`:

```text
option('dxvk_experimental_simd', type : 'boolean', value : false)
```

When enabled on x86 or x86_64:

- MSVC/clang-cl gets `/arch:AVX2` and `/clang:-mfma` when supported.
- GCC/Clang-style compilers get `-mavx2` and `-mfma` when supported.
- Non-x86 targets are rejected for this experimental path.
- If no AVX2 flags are supported, configuration errors out.

The experimental SIMD build currently wires selected translation units as SIMD-enabled static libraries, including:

- `src/util/util_matrix.cpp`
- `src/spirv/spirv_compression.cpp`
- `src/dxvk/dxvk_allocator.cpp`
- `src/dxvk/dxvk_constant_state.cpp`
- `src/dxvk/dxvk_descriptor_info.cpp`
- `src/dxvk/dxvk_util.cpp`
- `src/dxvk/dxvk_stats.cpp`
- `src/dxvk/dxvk_shader_key.cpp`
- `src/dxvk/dxvk_meta_clear.cpp`
- `src/dxvk/dxvk_pipemanager.cpp`
- `src/dxvk/dxvk_shader_builtin.cpp`

The D3D11 target also receives the experimental SIMD compiler arguments when the option is enabled.

Several header-only helpers contain AVX2 code paths that activate when included from AVX2-enabled translation units:

- `src/util/util_vector.h`
- `src/util/util_string.h`
- `src/util/util_lru.h`
- `src/util/util_bit.h`
- `src/dxvk/dxvk_hash.h`
- `src/dxvk/dxvk_graphics.h`
- `src/dxvk/dxvk_graphics_state.h`

## Implemented AVX2 Areas

### Matrix Operations

File: `src/util/util_matrix.cpp`

Status: implemented for the experimental SIMD build.

Implemented work:

- AVX2 matrix `operator+`
- AVX2 matrix `operator-`
- AVX2 scalar `operator*`
- AVX2 scalar `operator/`
- AVX2 component-wise `operator*`
- AVX2 in-place `operator+=`
- AVX2 in-place `operator-=`
- AVX2 transpose path
- AVX2 determinant helper work
- AVX2/FMA inverse path
- SIMD counter zone: `MatrixOps`

### Vector Helpers

File: `src/util/util_vector.h`

Status: implemented as header-only helpers.

Implemented work:

- `Vector4x2` helper for processing two `Vector4` values in one 256-bit register.
- Batch load/store helpers.
- Component-wise add, subtract, multiply, scalar multiply, and in-place variants.

### SPIR-V Decompression

File: `src/spirv/spirv_compression.cpp`

Status: implemented for the experimental SIMD build.

Implemented work:

- AVX2 block decode in `SpirvCompressedBuffer::decompress()`.
- Parallel shift/mask operations for packed compressed data.
- `_mm256_permutevar8x32_epi32` use where suitable.
- Scalar fallback for unsupported builds and remainder cases.
- SIMD counter zone: `SpirvDecompress`

### Image Data Packing

File: `src/dxvk/dxvk_util.cpp`

Status: implemented for the experimental SIMD build.

Implemented work:

- `copyRowAVX2` copies 32-byte chunks with `_mm256_loadu_si256` and `_mm256_storeu_si256`.
- Used by `packImageData` paths where row copying is suitable.
- Scalar fallback handles tails and non-AVX2 builds.
- SIMD counter zone: `ImagePacking`

### Hashing and Packed State Equality

Files:

- `src/dxvk/dxvk_hash.h`
- `src/dxvk/dxvk_graphics_state.h`
- `src/util/util_bit.h`

Status: implemented for packed graphics and compute pipeline-state hashing/equality.

Implemented work:

- `DxvkHashState4` batches four 64-bit hash lanes with AVX2.
- Packed graphics and compute pipeline-state hashes feed aligned 32-byte chunks into the SIMD hash helper.
- Final hash folds SIMD lanes with XOR.
- `bit::bcmpeq()` uses AVX2 byte comparison over 32-byte chunks where struct layout is suitable.

### Meta Shader and Shader Key Helpers

Files:

- `src/dxvk/dxvk_meta_clear.cpp`
- `src/dxvk/dxvk_shader_builtin.cpp`
- `src/dxvk/dxvk_shader_key.cpp`

Status: implemented for the experimental SIMD build.

Implemented work:

- `determineWorkgroupSize()` uses AVX2 comparison against candidate `VkImageViewType` values.
- `emitFormatVector()` uses a branch-minimized active-lane path.
- `DxvkShaderHash::eq()` uses `std::memcmp()` under AVX2 builds.
- `DxvkShaderHash::hash()` packs metadata and shader hash data into 64-bit chunks before hashing.
- SIMD counter zone: `ShaderOps`

### Pipeline Manager and Pipeline State Lookups

Files:

- `src/dxvk/dxvk_graphics.h`
- `src/dxvk/dxvk_graphics_state.h`
- `src/dxvk/dxvk_constant_state.cpp`

Status: implemented.

Implemented work:

- `DxvkGraphicsPipelineShaders::eq()` uses `std::memcmp()` under AVX2 builds.
- `DxvkGraphicsPipelineShaders::hash()` loads the first four shader pointers with `_mm256_loadu_si256`, extracts shader cookies, and hashes packed cookie pairs.
- Packed graphics and compute pipeline-state structs use AVX2 equality and hash batching.
- `DxvkBlendMode::normalize()` has an AVX2 comparison helper for blend-state normalization.
- Depth/stencil normalization remains scalar.
- SIMD counter zone: `PipelineOps`

### Descriptor Update List Processing

File: `src/dxvk/dxvk_descriptor_info.cpp`

Status: implemented for the experimental SIMD build.

Implemented work:

- `copy_nontemporal<32u>()` uses `_mm256_stream_si256`.
- `clear_nontemporal()` uses AVX2 streaming zero stores for 32-byte chunks.
- Descriptor copy/list paths are instrumented.
- SIMD counter zone: `DescriptorOps`

### Memory Allocator Page Management

File: `src/dxvk/dxvk_allocator.cpp`

Status: implemented for page-mask initialization and clearing.

Implemented work:

- AVX2 helper fills full 32-bit page-mask words eight at a time.
- `getPageAllocationMask()` uses AVX2 stores where the mask layout is naturally vectorizable.
- Free-list traversal, range merging, and pool allocation remain scalar because those paths mutate pointer/index structures.
- SIMD counter zone: `MemoryOps`

### String Processing

File: `src/util/util_string.h`

Status: implemented as guarded header-only AVX2 paths.

Implemented work:

- AVX2 `length()` fast paths for 1-byte and 2-byte strings.
- AVX2 ASCII-to-wide expansion in `transcodeString()`.
- Scalar fallback remains for unsupported encodings, tails, and non-AVX2 builds.
- SIMD counter zone: `MiscOps`

### LRU Cache Operations

File: `src/util/util_lru.h`

Status: implemented as a guarded recent-key fast path.

Implemented work:

- Four-entry recent-key hint cache for trivially copyable 64-bit keys.
- `_mm256_cmpeq_epi64` checks repeated keys before falling back to the authoritative `std::unordered_map` and `std::list`.
- Hint cache is updated on insert, touch, and remove to avoid stale iterators.
- SIMD counter zone: `MiscOps`

### Implicit Resolve Tracking

File: `src/dxvk/dxvk_implicit_resolve.cpp`

Status: implemented.

Implemented work:

- AVX2 age comparisons for resolve-view cleanup with `_mm256_cmpgt_epi64`.
- Four tracking IDs are processed per vectorized chunk.
- Scalar fallback handles tails and non-AVX2 builds.
- SIMD counter zone: `MiscOps`

### Statistics Counter Operations

File: `src/dxvk/dxvk_stats.cpp`

Status: implemented.

Implemented work:

- AVX2 `diff()`, `merge()`, and `reset()` loops process four 64-bit counters per iteration.
- Scalar tail loop handles category-count changes.
- SIMD counter zone: `MiscOps`

## SIMD Counter and HUD

The SIMD counter layer is implemented in `src/util/util_simd_perf.h/.cpp`.

It uses:

- `__rdtsc()` for cycle timing.
- Thread-local accumulation through `g_simdPerfLocal`.
- Atomic global accumulation through `g_simdPerfGlobal`.
- A flush cadence of 64 recorded scopes per thread-local accumulator.
- `snapshotSimdPerf()` to gather and reset accumulated counters for HUD display.

The counter macro is:

```cpp
DXVK_SIMD_PERF_SCOPE(ZoneName)
```

When `DXVK_SIMD_PERF=0`, the macro compiles to:

```cpp
((void)0)
```

Current counter zones:

| Enum | HUD Label | Tracks |
| --- | --- | --- |
| `MatrixOps` | `Matrix` | Matrix/vector math helper work. |
| `SpirvDecompress` | `SpirvDec` | SPIR-V compressed buffer decode work. |
| `ImagePacking` | `ImagePack` | CPU-side image row packing/copy helpers. |
| `DescriptorOps` | `Descriptor` | Descriptor copy, clear, and update-list helper work. |
| `ShaderOps` | `Shader` | Shader key/hash/builtin/meta shader helper work. |
| `PipelineOps` | `Pipeline` | Pipeline state hashing, equality, and normalization helpers. |
| `MemoryOps` | `Memory` | Allocator page-mask helper work. |
| `MiscOps` | `Misc` | Stats, string, LRU, and implicit resolve helper work. |

The HUD item is registered only in experimental SIMD builds:

```cpp
#if DXVK_SIMD_PERF
addItem<HudSimdPerfItem>("simd", ...)
#endif
```

Current HUD defaults:

- `simd_graph` defaults to `0`.
- `simd_breakdown` defaults to `0`.

So this shows only the compact total line:

```ini
dxvk.hud = simd
```

To show FPS, draw calls, and compact SIMD timing:

```ini
dxvk.hud = fps,drawcalls,simd,simd_graph=0,simd_breakdown=0
```

To enable the graph:

```ini
dxvk.hud = fps,drawcalls,simd,simd_graph=1
```

To enable the per-zone breakdown:

```ini
dxvk.hud = fps,drawcalls,simd,simd_breakdown=1
```

To enable both:

```ini
dxvk.hud = fps,drawcalls,simd,simd_graph=1,simd_breakdown=1
```

## Relationship to the D3D11 Governor

The AVX2/SIMD work is separate from the D3D11 frame governor.

The SIMD build still includes the governor options because it is built from the same source tree. Current governor options include:

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

The older plateau compatibility options also still exist:

```ini
d3d11.drawPlateauMode
d3d11.drawPlateauBaseline
d3d11.drawPlateauTargetPercent
d3d11.drawPlateauSkipMaxVertices
```

Current testing guidance for the settlement scene is to use the newer governor keys and avoid `d3d11.drawPlateauMode`.

Recommended active test config:

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

dxvk.hud = fps,drawcalls,simd,simd_graph=0,simd_breakdown=0
```

The governor HUD is surfaced through the draw-call HUD item:

- `Governor sleep:` shows early governor sleep and limiter sleep in microseconds.
- `Governor skip:` shows skipped draw calls when shedding is active.

## Deferred or Not Implemented

These are still candidates but are not implemented in the current tree:

- `src/dxvk/dxvk_staging.cpp`: memory-copy/reset specialization.
- `src/dxvk/dxvk_barrier.cpp`: barrier interval/tree traversal.
- `src/dxvk/dxvk_shader_io.cpp`: shader IO sorting and compatibility checks.
- `src/dxvk/dxvk_format.cpp`: format lookup/prefetch experiments.
- `src/dxvk/dxvk_meta_blit.cpp`: meta shader generation swizzle experiments.
- `src/dxvk/dxvk_meta_copy.cpp`: meta shader generation swizzle experiments.
- `src/dxvk/dxvk_meta_mipgen.cpp`: meta shader generation swizzle experiments.

## Removed After Regression Testing

The following draw-path experiments were removed or should remain out of the current AVX2 strategy because they touched stateful D3D11/DXVK draw submission paths and caused or risked frame-time regressions:

- AVX2 `HasDirtyGraphicsBindings()` replacement.
- AVX2 dirty binding mask early-outs in D3D11 context binding application.
- CPU-side per-draw frustum culling.
- Vertex-buffer bounds copying/scanning and `ComputeBounds()` support.
- AVX2 gather-based multi-draw batching checks.
- `DirectMultiDrawBatchSize` increase from 256 to 512.
- Draw-count downscaling through `d3d11.downscaleFactor`.
- Experimental config options `d3d11.mergeDrawCalls`, `d3d11.frustumCull`, `d3d11.occlusionCull`, and `d3d11.downscaleFactor`.

Current rule: keep speculative SIMD work out of the draw submission path unless it has narrow profiling data, a correctness proof, and a clean runtime gate.

## Implementation Guidelines

- Prefer SIMD in naturally batchable helper code.
- Avoid stateful draw-submission paths for SIMD experiments.
- Keep scalar fallbacks for every AVX2 path.
- Use unaligned loads/stores unless alignment is guaranteed.
- Avoid gathers in hot loops unless profiling proves they win.
- Avoid adding memory copies, locks, resource scans, or extra synchronization to render-thread paths.
- Instrument with `DXVK_SIMD_PERF_SCOPE(...)` only around the measured block.
- Use the SIMD HUD to validate whether a code path actually fires during gameplay.

## Testing Strategy

1. Build the regular clang profile.
2. Build the experimental SIMD profile with `DXVK_EXPERIMENTAL_SIMD=1`.
3. Test with the SIMD DLLs from `build_clang_simd`.
4. Use `dxvk.hud = fps,drawcalls,simd,simd_graph=0,simd_breakdown=0` for compact testing.
5. Enable `simd_breakdown=1` only when identifying active zones.
6. Benchmark Fallout 4 with consistent saves, camera direction, weather, time of day, and settlement load.
7. Compare frame-time lows and stutter, not only average FPS.

## Current Next Steps

1. Use the SIMD HUD in the Sim Settlements scene to confirm which zones are active.
2. Keep the newer D3D11 governor separate from AVX2 experiments.
3. Tune `d3d11.governorDrawBudget`, `d3d11.governorMaxSkipsPerFrame`, and `d3d11.governorSkipMaxVertices` for the 39 FPS settlement case.
4. Preserve only SIMD changes that improve real frame-time consistency.
5. Leave draw submission scalar unless future profiling proves a narrow AVX2 path is worth the risk.
