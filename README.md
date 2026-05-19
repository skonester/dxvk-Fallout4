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

Using this Windows-native, Clang-compiled DXVK build for Fallout 4 offers several benefits:

* **vs. Native D3D11 on Windows:** Significant reduction in CPU-bound draw call bottlenecks (e.g., downtown Boston stutters), leading to improved frame pacing and higher minimum framerates.
* **vs. Linux (Proton + DXVK):** Since the game runs natively on Windows, it completely bypasses the system call translation overhead introduced by Wine/Proton on Linux. This results in comparable or better raw performance and lower latency, while utilizing the same Vulkan optimization advantages.
* **Clang-cl Optimization:** Compiling with `clang-cl` matches the MSVC ABI exactly, ensuring clean integration with the Windows Universal C Runtime (UCRT) and Windows thread scheduling.

---

## Clang Compile-Time Optimizations (AVX2, FMA, & Fast-Math)

Both the local `build_dx11.ps1` script and the GitHub Actions release workflows are configured to build with advanced compiler optimizations for modern CPUs:

* **AVX2 (`-mavx2`):** Generates optimized Vector extensions for modern x86-64 processors (Intel Haswell / AMD Zen or newer), enhancing vertex processing and draw-call translation throughput.
* **FMA (`-mfma`):** Utilizes Fused Multiply-Add instructions to perform floating-point multiply-accumulate operations in a single step, improving mathematical precision and execution speed.
* **Fast-Math (`/fp:fast`):** Instructs Clang-CL to perform aggressive mathematical optimizations, allowing faster hardware vectorization for floating-point calculations.

These options compile directly into the `d3d11.dll` and `dxgi.dll` binaries, yielding higher performance in CPU-limited scenarios.
