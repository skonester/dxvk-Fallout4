#include "dxvk_stats.h"

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace dxvk {
  
  DxvkStatCounters::DxvkStatCounters() {
    this->reset();
  }
  
  
  DxvkStatCounters::~DxvkStatCounters() {
    
  }
  
  
  DxvkStatCounters DxvkStatCounters::diff(const DxvkStatCounters& other) const {
    DxvkStatCounters result;
#if defined(__AVX2__)
    uint64_t* dst = result.m_counters.data();
    const uint64_t* src1 = m_counters.data();
    const uint64_t* src2 = other.m_counters.data();
    size_t i = 0;
    size_t size = m_counters.size();
    size_t simdBits = size & ~size_t(3);
    for (; i < simdBits; i += 4) {
      __m256i s1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src1 + i));
      __m256i s2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src2 + i));
      __m256i res = _mm256_sub_epi64(s1, s2);
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), res);
    }
    for (; i < size; i++)
      dst[i] = src1[i] - src2[i];
#else
    for (size_t i = 0; i < m_counters.size(); i++)
      result.m_counters[i] = m_counters[i] - other.m_counters[i];
#endif
    return result;
  }
  
  
  void DxvkStatCounters::merge(const DxvkStatCounters& other) {
#if defined(__AVX2__)
    uint64_t* dst = m_counters.data();
    const uint64_t* src = other.m_counters.data();
    size_t i = 0;
    size_t size = m_counters.size();
    size_t simdBits = size & ~size_t(3);
    for (; i < simdBits; i += 4) {
      __m256i d = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(dst + i));
      __m256i s = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
      __m256i res = _mm256_add_epi64(d, s);
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), res);
    }
    for (; i < size; i++)
      dst[i] += src[i];
#else
    for (size_t i = 0; i < m_counters.size(); i++)
      m_counters[i] += other.m_counters[i];
#endif
  }
  
  
  void DxvkStatCounters::reset() {
#if defined(__AVX2__)
    uint64_t* dst = m_counters.data();
    __m256i zero = _mm256_setzero_si256();
    size_t i = 0;
    size_t size = m_counters.size();
    size_t simdBits = size & ~size_t(3);
    for (; i < simdBits; i += 4) {
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), zero);
    }
    for (; i < size; i++)
      dst[i] = 0;
#else
    for (size_t i = 0; i < m_counters.size(); i++)
      m_counters[i] = 0;
#endif
  }
  
}
