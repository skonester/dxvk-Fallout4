#include "util_simd_perf.h"

#if DXVK_SIMD_PERF

#include <windows.h>

namespace dxvk {

  thread_local SimdPerfAccumulator g_simdPerfLocal;
  SimdPerfGlobalAccumulator g_simdPerfGlobal;

  void SimdPerfAccumulator::flush() {
    for (uint32_t i = 0; i < uint32_t(SimdPerfZone::Count); i++) {
      if (cycles[i] > 0) {
        g_simdPerfGlobal.cycles[i].fetch_add(cycles[i], std::memory_order_relaxed);
        cycles[i] = 0;
      }
    }
    count = 0;
  }

  SimdPerfAccumulator::~SimdPerfAccumulator() {
    flush();
  }

  uint64_t calibrateRdtscFrequency() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    
    LARGE_INTEGER start_qpc, end_qpc;
    uint64_t start_tsc, end_tsc;
    
    QueryPerformanceCounter(&start_qpc);
    start_tsc = __rdtsc();
    
    // Wait for approximately 100ms
    Sleep(100);
    
    end_tsc = __rdtsc();
    QueryPerformanceCounter(&end_qpc);
    
    uint64_t elapsed_qpc = end_qpc.QuadPart - start_qpc.QuadPart;
    uint64_t elapsed_tsc = end_tsc - start_tsc;
    
    // Return ticks per second
    return (elapsed_tsc * frequency.QuadPart) / elapsed_qpc;
  }

  SimdPerfAccumulator snapshotSimdPerf() {
    g_simdPerfLocal.flush();

    SimdPerfAccumulator result;
    for (uint32_t i = 0; i < uint32_t(SimdPerfZone::Count); i++) {
      result.cycles[i] = g_simdPerfGlobal.cycles[i].exchange(0, std::memory_order_relaxed);
    }
    return result;
  }

}

#endif // DXVK_SIMD_PERF