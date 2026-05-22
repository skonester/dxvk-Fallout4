# AVX2 SIMD Performance Telemetry Overlay

The experimental-SIMD build of DXVK for Fallout 4 includes a real-time, low-overhead cycle-level performance telemetry layer. This layer measures the execution overhead of AVX2-optimized hot paths on the CPU.

---

## What It Measures (8 Performance Zones)

Telemetry is divided into 8 distinct execution zones within DXVK:

| Zone | Label | What it Tracks |
| :--- | :--- | :--- |
| **Matrix** | `Matrix` | CPU-side vertex, coordinate, projection, and animation transform matrix calculations. |
| **SpirvDec** | `SpirvDec` | Shader program decompression routines for background SPIR-V compile pipelines. |
| **ImagePack** | `ImagePack` | Converting and packing raw texture frames into Vulkan-compatible formats on the fly. |
| **Descriptor** | `Descriptor` | Nontemporal (cache-bypass) copy/clear actions updating descriptor structures for draw submissions. |
| **Shader** | `Shader` | Hashing and equality verification for shader state keys and shader parameter structures. |
| **Pipeline** | `Pipeline` | Graphics pipeline state hashing, blend normalizations, and cache lookups. |
| **Memory** | `Memory` | CPU-side allocation mask queries tracking Vulkan memory pages. |
| **Misc** | `Misc` | String manipulation helpers, implicit cache resolves, and LRU eviction queues. |

---

## Configuration & HUD Options (`dxvk.conf`)

You can configure the overlay directly by editing the `dxvk.hud` line in `dxvk.conf` (place this file in the directory containing `Fallout4.exe`):

### 1. Show Everything (FPS + SIMD Header + Graph + Breakdown)
Shows a live cyan sparkline graph of CPU timing and a line-item breakdown of each zone:
```ini
dxvk.hud = fps,simd
```

### 2. Hide the Sparkline Graph (Text Only)
Hides the sparkline graph, showing only the text stats and breakdown (ideal for space-constrained overlays):
```ini
dxvk.hud = simd,simd_graph=0
```

### 3. Hide the Detailed Breakdown (Graph Only)
Shows only the total SIMD execution time (`µs/frame`) and the transparent graph:
```ini
dxvk.hud = simd,simd_breakdown=0
```

### 4. Minimal Display (Header Text Line Only)
Shows only the total execution time line:
```ini
dxvk.hud = simd,simd_graph=0,simd_breakdown=0
```

---

## Key Technical Details

### Low-Overhead Telemetry
The telemetry uses the CPU's hardware Time Stamp Counter (`__rdtsc()`) intrinsic to measure clock cycles. It takes ~5–10 CPU cycles per measurement, causing virtually zero impact on frame pacing. Ticks are calibrated at startup against the Query Performance Counter (QPC) to yield real-time microseconds (`µs`).

### Transparent Sparkline Graph
The graph has a fully transparent background, blending directly over the game viewport without drawing an opaque black box on the screen.

### Timing Jitter & Idle Behavior
If you stand idle in the game, you may still see SIMD time active. Even when idle, Fallout 4 continues to update bones, calculate wind sway, process ambient particles, and update camera transforms. Jitter or elevated `µs` values during idle states can also occur due to CPU clock frequency scaling (speed shifting/downclocking) or OS scheduler context switching during low-load situations.
