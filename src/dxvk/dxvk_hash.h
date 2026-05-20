#pragma once

#include <cstddef>

#include "../util/util_env.h"
#include "../util/util_bit.h"

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
#if defined(DXVK_ARCH_X86_64)
      m_value = _mm_crc32_u64(m_value, hash);
#elif defined(DXVK_ARCH_X86)
      m_value = _mm_crc32_u32(m_value, hash);
#else
      m_value ^= hash;
      m_value *= Prime;
#endif
    }

    operator size_t () const {
      return m_value;
    }

  private:

    size_t m_value = Offset;

  };

}
