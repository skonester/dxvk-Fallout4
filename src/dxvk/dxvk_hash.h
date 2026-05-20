#pragma once

#include <cstddef>
#include <cstdint>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

#include "../util/util_env.h"

namespace dxvk {

  struct DxvkEq {
    template<typename T>
    size_t operator () (const T& a, const T& b) const {
      return a.eq(b);
    }
  };

  struct DxvkHash {
    template<typename T>
    size_t operator () (const T& object) const {
      return object.hash();
    }
  };
  
  class DxvkHashState {
    static constexpr size_t Offset = env::is32BitHostPlatform()
      ? size_t(0x811c9dc5u)
      : size_t(0xcbf29ce484222325ull);

    static constexpr size_t Prime = env::is32BitHostPlatform()
      ? size_t(0x01000193u)
      : size_t(0x00000100000001b3ull);
  public:

    void add(size_t hash) {
      m_value ^= hash;
      m_value *= Prime;
    }

    operator size_t () const {
      return m_value;
    }

  private:

    size_t m_value = Offset;

  };

  #if defined(__AVX2__) && defined(DXVK_ARCH_X86_64)
  class DxvkHashState4 {
    static constexpr uint64_t Offset = 0xcbf29ce484222325ull;
    static constexpr uint64_t Prime  = 0x00000100000001b3ull;
  public:

    void add(uint64_t h0, uint64_t h1, uint64_t h2, uint64_t h3) {
      __m256i hash = _mm256_set_epi64x(int64_t(h3), int64_t(h2), int64_t(h1), int64_t(h0));
      m_value = mulLo64(_mm256_xor_si256(m_value, hash), m_prime);
    }

    void add(__m256i hash) {
      m_value = mulLo64(_mm256_xor_si256(m_value, hash), m_prime);
    }

    size_t get(uint32_t index) const {
      alignas(32) uint64_t values[4];
      _mm256_store_si256(reinterpret_cast<__m256i*>(values), m_value);
      return size_t(values[index]);
    }

  private:

    static __m256i mulLo64(__m256i a, __m256i b) {
      const __m256i mask = _mm256_set1_epi64x(0xffffffffull);

      __m256i aLo = _mm256_and_si256(a, mask);
      __m256i bLo = _mm256_and_si256(b, mask);
      __m256i aHi = _mm256_srli_epi64(a, 32);
      __m256i bHi = _mm256_srli_epi64(b, 32);

      __m256i loLo = _mm256_mul_epu32(aLo, bLo);
      __m256i hiLo = _mm256_mul_epu32(aHi, bLo);
      __m256i loHi = _mm256_mul_epu32(aLo, bHi);

      return _mm256_add_epi64(loLo, _mm256_slli_epi64(_mm256_add_epi64(hiLo, loHi), 32));
    }

    __m256i m_value = _mm256_set1_epi64x(Offset);
    __m256i m_prime = _mm256_set1_epi64x(Prime);

  };
  #endif

}
