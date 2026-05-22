# Fallout 4 Vulkan (DXVK)

This repository is a continuation of the **Fallout 4 Vulkan** project hosted on Nexus Mods, optimized for Windows native compilation. It provides a customized version of DXVK focusing on Direct3D 11 (`d3d11.dll`) and DXGI (`dxgi.dll`) built natively with Windows tools.

---

## Build System & Requirements

This project is configured to build natively on Windows using only Windows-based tools, avoiding standard cross-compilation toolchains such as Wine or MinGW.

### Requirements
* **Visual Studio 2022** (Build Tools or Enterprise/Professional/Community) with C++ development workloads.
* **Clang/LLVM for Windows** (`clang-cl`) compiler.
* **Python 3.11+** with the `meson` build package installed.
* **Ninja** build system.
* **vcpkg** package manager with `glslang[tools]` installed:
  ```powershell
  vcpkg install glslang[tools]
  ```

---

## How to Build

We provide a PowerShell automation script `build_dx11.ps1` at the root of the repository that handles the entire build process.

### Using the Automated Script
To configure and compile the Direct3D 11 and DXGI binaries automatically:
1. Open PowerShell.
2. Run the script:
   ```powershell
   .\build_dx11.ps1
   ```

The script will automatically:
1. Locate Visual Studio 2022 and load the x64 Developer Command Prompt environment variables (including `INCLUDE`, `LIB`, and `PATH`).
2. Locate the `glslangValidator.exe` binary installed via `vcpkg`.
3. Set the compiler to `clang-cl`.
4. Initialize the Meson configuration targeting `d3d11` and `dxgi` (disabling legacy APIs like `d3d8`, `d3d9`, and `d3d10`).
5. Execute the build using `ninja`.

### Output Binaries
Upon completion, the compiled Windows-native DLLs are saved under:
* `build_clang/src/d3d11/d3d11.dll`
* `build_clang/src/dxgi/dxgi.dll`

---

## Manual Build Instructions

If you prefer to run the build steps manually in a Developer Command Prompt (x64):

1. **Configure the build directory:**
   ```cmd
   python -m mesonbuild.mesonmain setup build_clang --backend ninja -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false
   ```
2. **Compile using Ninja:**
   ```cmd
   ninja -C build_clang
   ```

---

## Performance Comparison (Windows vs. Linux / Proton)

Using this Windows-native, Clang-compiled DXVK build for Fallout 4 can help in CPU-bound scenes by moving D3D11 command translation onto DXVK's Vulkan backend:

* **vs. Native D3D11 on Windows:** DXVK can reduce some driver-side D3D11 overhead by translating work to Vulkan command submission. This is most relevant in CPU-limited areas with many objects, shadows, decals, and state changes, such as downtown Boston.
* **vs. Linux / Proton + DXVK:** These DLLs are Windows PE binaries and run through Wine/Proton on Linux. They should not require Linux-native DXVK `.so` files, but final behavior still depends on Wine/Proton, the Vulkan driver, shader cache state, and CPU scheduling. Do not assume Windows and Linux results will be identical.
* **Clang-cl Optimization:** Compiling with `clang-cl` keeps the MSVC ABI while allowing the project to use LLVM's optimizer. The build uses the static MSVC runtime setting, so the generated DLLs are intended to avoid extra MSVC redistributable dependencies.

---

## Experimental SIMD / AVX2 Build

This repository includes an optional experimental SIMD build path for selected DXVK hot paths. Enable it with:

```powershell
$env:DXVK_EXPERIMENTAL_SIMD = "1"
$env:DXVK_BUILD_DIR = "build_clang_simd"
.\build_dx11.ps1
```

The experimental build uses AVX2-capable compiler flags and compiles selected translation units with SIMD-specific code paths. It requires an AVX2-capable x86-64 CPU, such as Intel Haswell or newer, or AMD Excavator / Zen or newer.

### Why This Can Help Fallout 4 Draw Calls

Fallout 4's heavy scenes are often limited by CPU-side rendering work rather than raw GPU shader throughput. Downtown Boston is a common stress case because the renderer has to process many visible objects, dynamic buffers, state changes, draw submissions, shadows, decals, and visibility results in a dense area. DXVK already helps by changing the D3D11 driver path; the SIMD build tries to shave overhead from repeated CPU-side helper work around that translation.

The current experimental SIMD path targets small but frequently repeated operations, including:

* Matrix and vector math used by rendering helpers.
* SPIR-V decompression and shader-key/hash helpers.
* Image row packing and descriptor update copies.
* Statistics counter updates.
* UTF/string helper scans used by utility paths.
* Allocator page-mask fills used by memory/debug accounting paths.
* LRU recent-key checks for trivially copyable 64-bit keys.
* Blend-state normalization comparisons.

These optimizations are not a replacement for engine fixes, precombines, shadow-distance tuning, or reducing script/mod load. They also do not reduce the number of draw calls the game submits. The goal is narrower: reduce some CPU cycles spent per repeated DXVK-side operation so frame pacing can improve when the game is already close to the CPU limit.

Expected impact is workload-dependent. The best-case improvement is usually smoother minimum frame times in dense areas; the realistic worst case is no measurable change. Treat this as an experimental performance profile, not a guaranteed FPS multiplier.

---

## Optimized DXVK Configuration (`dxvk.conf`)

An optimized `dxvk.conf` configuration template is provided at the root and under `/conf`. Copy this file into the game directory containing your `Fallout4.exe` to enable the following performance presets:

* **Frame Rate Limiter (`dxgi.maxFrameRate = 60`):** Limits frame presentation to 60 FPS. Essential for preventing engine/physics speed-up bugs in Fallout 4 and maintaining uniform frame times.
* **Low Input Latency (`dxgi.maxFrameLatency = 1`):** Limits the CPU flip queue to a single frame. Minimizes mouse latency and keeps gameplay highly responsive.
* **VSync (`dxgi.syncInterval = 1`):** Synchronizes frames with the monitor refresh rate to completely eliminate screen tearing.
* **Resource Caching (`d3d11.cachedDynamicResources = a`):** Caches dynamic Constant, Vertex, and Index buffers to significantly alleviate driver and CPU rendering bottlenecks.
* **Relaxed Barriers (`d3d11.relaxedBarriers = True`):** Enables GPU pipeline overlap for optimized command execution.
* **Async Shader Compilation (`dxvk.numCompilerThreads = 0`):** Instructs the compiler to utilize all available CPU threads for background shader compilation, avoiding stuttering when loading new graphics assets.
* **HUD Overlay Configuration (`dxvk.hud = ...`):** Configures display overlay items. You can customize the experimental SIMD performance HUD components:
  - `dxvk.hud = simd` shows the header text, the transparent sparkline graph, and the detailed zone breakdown.
  - `dxvk.hud = simd,simd_graph=0` hides the sparkline graph, showing only the text stats.
  - `dxvk.hud = simd,simd_breakdown=0` hides the detailed zone breakdown, showing only the header and the graph.
  - `dxvk.hud = fps,simd` displays both the FPS counter and the SIMD performance item.

