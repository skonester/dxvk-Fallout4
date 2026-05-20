# Safe AVX2/FMA Implementation Plan

Goal: reintroduce targeted AVX2/FMA optimizations without changing DXVK behavior, ABI, object layout, or memory alignment assumptions. Each phase should build, run in Fallout 4, and be easy to revert independently.

## Ground Rules

- Keep every optimization in a small, reviewable commit.
- Prefer local helper functions over broad rewrites of render state, hashing, descriptors, or pipeline keys.
- Do not enable global `-ffast-math`, `/fp:fast`, or broad FMA contraction for the whole project.
- Do not compare or hash whole structs with `memcmp` or CRC unless all padding bytes are known initialized and the original equality semantics are byte-exact.
- Do not use aligned or streaming stores unless the destination alignment is proven with code-level guarantees.
- Prefer `_mm_loadu_*` and `_mm_storeu_*` for general-purpose vector/matrix code.
- Keep scalar fallback code as the reference implementation.
- Gate AVX2/FMA code with compile-time checks and, where needed, runtime CPU feature checks.

## Known Bad Patterns To Avoid

- `_mm256_stream_si256` on memory that is only 16-byte aligned.
- `_mm256_store_si256` / `_mm256_load_si256` unless 32-byte alignment is guaranteed.
- Replacing `std::memcpy` with typed pointer writes where alignment or aliasing is uncertain.
- Rewriting pipeline state equality/hash functions to read padding bytes.
- Changing global compiler flags in `meson.build` that affect all floating-point behavior.

## Phase 1: Baseline And Safety Harness

- [x] Keep current stable build as the baseline.
- [x] Confirm `build_dx11.ps1` performs a clean configure when `build_clang` belongs to another source tree.
- [x] Add a short `README` or section in this plan with the exact game smoke test steps.
- [x] Build once with no AVX2/FMA changes and record DLL sizes and game behavior.

Validation:

- `powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1`
- Copy `build_clang/src/d3d11/d3d11.dll` and `build_clang/src/dxgi/dxgi.dll` to the Fallout 4 test folder.
- Launch Fallout 4 and verify it reaches gameplay without crashing.

## Build And Smoke Test Steps

Stable build:

- Run `powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1`.
- Test `build_clang/src/d3d11/d3d11.dll`.
- Test `build_clang/src/dxgi/dxgi.dll`.

Experimental SIMD build:

- Run `$env:DXVK_BUILD_DIR='build_clang_simd'; $env:DXVK_EXPERIMENTAL_SIMD='1'; powershell.exe -ExecutionPolicy Bypass -File .\build_dx11.ps1`.
- Test `build_clang_simd/src/d3d11/d3d11.dll`.
- Test `build_clang_simd/src/dxgi/dxgi.dll`.

Fallout 4 smoke test:

- Launch to the main menu.
- Load a save into gameplay.
- Visit an indoor cell.
- Visit an outdoor worldspace.
- Rotate the camera quickly for at least 30 seconds.
- Trigger shader/pipeline activity by changing areas or loading a save with different lighting/weather.
- Check the DXVK log for new errors, device-loss messages, or descriptor/pipeline warnings.

## Phase 2: Safe Vector Helpers

Candidate file:

- `src/util/util_vector.h`

Scope:

- Reintroduce SSE-only float `Vector4` helpers using unaligned loads/stores:
  - [x] `operator+`
  - [x] `operator-`
  - [x] scalar `operator*`
  - [x] component `operator*`
  - [x] `operator+=`
  - [x] `operator-=`
  - [x] `operator*=`

Rules:

- Use `_mm_loadu_ps` and `_mm_storeu_ps`.
- Do not use `_mm_dp_ps` unless SSE4.1 availability is explicitly guaranteed.
- Do not change `Vector4` size, alignment, constructors, or public layout.
- Keep scalar code as fallback.

Validation:

- [x] Build.
- [x] Run Fallout 4 smoke test.
- If stable, keep as a separate commit.

## Phase 3: Safe Matrix Bulk Operations

Candidate file:

- `src/util/util_matrix.cpp`

Scope:

- Reintroduce AVX2 paths only for byte-contiguous matrix operations:
  - [x] `Matrix4::operator+`
  - [x] `Matrix4::operator-`
  - [x] scalar `Matrix4::operator*`
  - [x] scalar `Matrix4::operator/`
  - [x] `hadamardProduct`

Rules:

- Use `_mm256_loadu_ps` and `_mm256_storeu_ps`.
- Avoid streaming stores.
- Avoid changing `Matrix4::operator*(const Matrix4&)` in this phase.
- Avoid changing `Matrix4::operator*(const Vector4&)` in this phase.

Validation:

- [x] Build.
- [x] Build default non-AVX2 configuration.
- [x] Build experimental AVX2 configuration with `DXVK_EXPERIMENTAL_SIMD=1`.
- [x] Run Fallout 4 smoke test with `build_clang_simd` DLLs.
- [x] Compare logs for new warnings/errors.
- Keep as a separate commit only if stable.

## Phase 4: FMA Matrix Multiply Experiment

Candidate file:

- `src/util/util_matrix.cpp`

Scope:

- Add optional FMA path for:
  - [x] `Matrix4::operator*(const Matrix4&)`
  - [x] `Matrix4::operator*(const Vector4&)`

Rules:

- Do not enable global fast-math.
- Use explicit `_mm_fmadd_ps` only inside the helper.
- Expect tiny floating-point differences; test visually and for crashes.
- Keep this phase separate from AVX2 bulk operations.

Validation:

- [x] Build default non-FMA configuration.
- [x] Build experimental FMA configuration with `DXVK_EXPERIMENTAL_SIMD=1`.
- [x] Run Fallout 4 smoke test.
- Test several scenes:
  - [x] Main menu.
  - [x] Indoor cell.
  - [x] Outdoor world.
  - [x] Combat or fast camera movement.
- Revert this phase if there are visual artifacts, unstable camera/projection behavior, or crashes.

## Phase 5: Descriptor Copy Investigation

Candidate file:

- `src/dxvk/dxvk_descriptor_info.cpp`

Scope:

- [x] Investigate descriptor copy hot paths only after vector/matrix changes are stable.
- [x] Add AVX2 copy path for 32-byte-aligned descriptor ranges with sizes that are multiples of 32.
- [x] Compile descriptor-info SIMD path only in the experimental build.

Rules:

- [x] Do not use `_mm256_stream_si256` unless destination alignment is guaranteed to be 32 bytes.
- If AVX2 is used, either:
  - [x] require `alignment >= 32`, or
  - use unaligned `_mm256_storeu_si256`.
- [x] Preserve current generic copy behavior for uncertain alignment.
- [ ] Benchmark or profile before keeping changes.

Validation:

- [x] Build default configuration.
- [x] Build experimental AVX2 descriptor-copy configuration.
- [x] Run Fallout 4 smoke test.
- [ ] Stress scene changes and shader/pipeline creation.

## Phase 6: Optional Build Flags

Scope:

- [x] Avoid project-wide AVX2/FMA flags until code-level optimization is proven stable.
- [x] Add a Meson option for an experimental optimized build, disabled by default.
- [x] Compile only `src/util/util_matrix.cpp` with AVX2 when the experimental option is enabled.

Rules:

- [x] No default `-ffast-math`, `/fp:fast`, or global FMA contraction.
- [x] Any new option must be opt-in and clearly named experimental.

Possible option:

- [x] `-Ddxvk_experimental_simd=true`

Validation:

- [x] Default build remains stable.
- [x] Experimental build compiles separately.
- [x] Experimental build is tested in Fallout 4.

## Rollback Strategy

- Each phase should be one commit.
- If Fallout 4 crashes, revert only the latest phase.
- If a phase needs more work, disable it behind a local macro before trying a narrower version.

## Initial Recommendation

The implemented SIMD set is now intentionally limited to vector helpers, matrix operations, and a guarded descriptor copy path. Further descriptor work should wait for profiling evidence.
