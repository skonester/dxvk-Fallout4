# DXVK Fallout 4 Stabilization Plan

## Summary

This is the active roadmap for the Fallout 4 DXVK build.

The old AVX2/FMA Phases 1-6 are treated as completed historical work. The current goal is not to add more speculative SIMD. The goal is to keep the native Windows `clang-cl` D3D11/DXGI build reproducible, keep experimental SIMD opt-in, and only retain optimizations that survive both build validation and Fallout 4 smoke testing.

## Known Good State

- The stable build path is the native Windows `clang-cl` build driven by `build_dx11.ps1`.
- The default build must remain non-experimental unless `DXVK_EXPERIMENTAL_SIMD=1` is set.
- `llvm-lib.exe`, `lld-link.exe`, and `llvm-rc.exe` must be discoverable without user-specific hardcoded paths.
- The SPIR-V append byte-count fix is correctness work, not an optimization experiment.
- AVX2 descriptor copy must avoid aligned or streaming stores unless alignment is guaranteed in code.
- Experimental SIMD DLLs must be built and tested separately from the stable build DLLs.

## Build Reliability

`build_dx11.ps1` is the supported build entry point for D3D11 and DXGI.

The script should keep discovering tools portably:

- Visual Studio C++ build environment through `vcvarsall.bat`.
- LLVM tools through `PATH`, common install locations, or `DXVK_LLVM_BIN`.
- `glslangValidator.exe` through `PATH`, vcpkg-style installs, or `DXVK_GLSLANG_DIR`.
- Meson through `meson.exe`, `python.exe -m mesonbuild.mesonmain`, or `py.exe -3 -m mesonbuild.mesonmain`.

Supported build knobs:

- `DXVK_BUILD_DIR`: choose a build directory.
- `DXVK_RECONFIGURE=1`: force Meson reconfiguration.
- `DXVK_EXPERIMENTAL_SIMD=1`: enable the opt-in SIMD build.
- `DXVK_LLVM_BIN`: explicitly point to LLVM `bin`.
- `DXVK_GLSLANG_DIR`: explicitly point to the directory containing `glslangValidator.exe`.

Stable build:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build_dx11.ps1
```

Experimental SIMD build:

```powershell
$env:DXVK_BUILD_DIR='build_clang_simd'
$env:DXVK_EXPERIMENTAL_SIMD='1'
$env:DXVK_RECONFIGURE='1'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build_dx11.ps1
```

## Runtime Stability

Fallout 4 game testing is the deciding validation step for D3D11/DXGI changes.

Smoke test procedure:

- Copy the candidate `d3d11.dll` and `dxgi.dll` into the Fallout 4 test folder.
- Launch Fallout 4 to the main menu.
- Load a save into gameplay.
- Visit an indoor cell.
- Visit an outdoor worldspace.
- Rotate the camera quickly for at least 30 seconds.
- Trigger shader and pipeline activity by changing areas or loading a save with different lighting or weather.
- Check DXVK logs for new errors, device-loss messages, descriptor warnings, or pipeline warnings.

Acceptance criteria:

- Stable build compiles and works in Fallout 4.
- Experimental SIMD build compiles and does not crash or visibly corrupt rendering.
- Stable and experimental DLLs are tested separately.
- Any runtime failure is treated as more important than a successful compile.

## SIMD Containment

Experimental SIMD remains opt-in through `DXVK_EXPERIMENTAL_SIMD=1`.

Current SIMD policy:

- Keep vector and matrix SIMD only if stable in game.
- Keep descriptor SIMD only with unaligned AVX2 stores and successful Fallout 4 testing.
- Do not add new SIMD hot paths without profiling evidence and a separate smoke-tested build.
- Do not use `_mm256_stream_si256`, `_mm256_store_si256`, or `_mm256_load_si256` unless 32-byte alignment is guaranteed in code.
- Prefer `_mm_loadu_*`, `_mm_storeu_*`, `_mm256_loadu_*`, and `_mm256_storeu_*` for general-purpose vector and matrix code.
- Do not enable project-wide `-ffast-math`, `/fp:fast`, or broad FMA contraction.

Current SIMD risk register:

- Descriptor updates are the highest-risk SIMD area because they interact with runtime-visible descriptor memory.
- Matrix FMA can introduce tiny floating-point differences, so visual testing matters even when builds pass.
- SPIR-V byte copying should remain correctness-focused and should not become a speculative SIMD parser rewrite.

If the experimental SIMD build breaks the game again, first disable the descriptor-info SIMD split while leaving matrix SIMD enabled. If that is still unstable, fall back to the stable build and reintroduce one SIMD area at a time.

## Cleanup Notes

- `clang-cl-dxvk-optimized.ini` is optional manual investigation config, not the main build path.
- `build_dx11.backup.ps1` is a temporary backup and should eventually be removed once the portable script is trusted.
- Old MinGW and optimization-remark probe plans are retired.

## Next Decisions

- Keep `build_dx11.ps1` as the single normal build entry point.
- Keep experimental SIMD separate in `build_clang_simd`.
- Do not expand SIMD until both stable and experimental builds have fresh Fallout 4 smoke-test results.
- Once the portable build script is trusted on another machine, remove the backup script and update the README with the final build commands.
