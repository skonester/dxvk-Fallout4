#pragma once

#include <buildenv.h>

#if DXVK_SIMD_PERF

#include <cstdint>
#include <intrin.h>
#include <array>
#include <atomic>

namespace dxvk {

  enum class SimdPerfZone : uint32_t {
    MatrixOps,        // util_matrix.cpp
    SpirvDecompress,  // spirv_compression.cpp
    ImagePacking,     // dxvk_util.cpp
    DescriptorOps,    // dxvk_descriptor_info.cpp
    ShaderOps,        // dxvk_shader_key, dxvk_shader_builtin, dxvk_meta_clear
    PipelineOps,      // dxvk_graphics.h, dxvk_constant_state
    MemoryOps,        // dxvk_allocator.cpp
    MiscOps,          // dxvk_stats, util_string, util_lru, dxvk_implicit_resolve
    Count
  };

  struct SimdPerfAccumulator {
    std::array<uint64_t, uint32_t(SimdPerfZone::Count)> cycles = {};
    uint32_t count = 0;

    void addCycles(SimdPerfZone zone, uint64_t c) {
      cycles[uint32_t(zone)] += c;
      if (++count >= 64) {
        flush();
      }
    }

    void flush();
    void reset() {
      cycles.fill(0);
      count = 0;
    }

    ~SimdPerfAccumulator();
  };

  struct SimdPerfGlobalAccumulator {
    std::array<std::atomic<uint64_t>, uint32_t(SimdPerfZone::Count)> cycles = {};
    SimdPerfGlobalAccumulator() {
      for (auto& c : cycles) {
        c.store(0, std::memory_order_relaxed);
      }
    }
  };

  // Thread-local accumulator — each thread sums independently
  extern thread_local SimdPerfAccumulator g_simdPerfLocal;
  extern SimdPerfGlobalAccumulator g_simdPerfGlobal;

  // RAII scope guard
  struct SimdPerfScope {
    SimdPerfZone zone;
    uint64_t start;

    SimdPerfScope(SimdPerfZone z)
    : zone(z), start(__rdtsc()) { }

    ~SimdPerfScope() {
      uint64_t elapsed = __rdtsc() - start;
      g_simdPerfLocal.addCycles(zone, elapsed);
    }
  };

  // Frequency calibration (call once at startup)
  uint64_t calibrateRdtscFrequency();

  // Snapshot: called once per frame from the main thread
  // Merges thread-local accumulators and returns the frame total
  SimdPerfAccumulator snapshotSimdPerf();

}

#define DXVK_SIMD_PERF_SCOPE_IMPL(zone, line) \
  dxvk::SimdPerfScope _simdPerfScope##line(dxvk::SimdPerfZone::zone)
#define DXVK_SIMD_PERF_SCOPE(zone) \
  DXVK_SIMD_PERF_SCOPE_IMPL(zone, __LINE__)

#else
#define DXVK_SIMD_PERF_SCOPE(zone) ((void)0)
#endif