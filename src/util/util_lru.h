#include <algorithm>
#include <cstdint>
#include <cstring>
#include <list>
#include <type_traits>
#include <unordered_map>

#include "util_bit.h"
#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace dxvk {

  template<typename T>
  class lru_list {

  public:
    typedef typename std::list<T>::const_iterator const_iterator;

    void insert(T value) {
#if defined(__AVX2__)
      if constexpr (UseRecentCache) {
        uint32_t recentIndex = findRecent(value);

        if (recentIndex < m_recentCount) {
          m_list.erase(m_recentIters[recentIndex]);
          m_list.push_back(value);

          auto iter = m_list.cend();
          iter--;

          m_cache[value] = iter;
          removeRecent(value);
          addRecent(value, iter);
          return;
        }
      }
#endif

      auto cacheIter = m_cache.find(value);
      if (cacheIter != m_cache.end()) {
#if defined(__AVX2__)
        if constexpr (UseRecentCache)
          removeRecent(value);
#endif
        m_list.erase(cacheIter->second);
      }

      m_list.push_back(value);
      auto iter = m_list.cend();
      iter--;
      m_cache[value] = iter;

#if defined(__AVX2__)
      if constexpr (UseRecentCache)
        addRecent(value, iter);
#endif
    }

    void remove(const T& value) {
      auto cacheIter = m_cache.find(value);
      if (cacheIter == m_cache.end())
        return;

#if defined(__AVX2__)
      if constexpr (UseRecentCache)
        removeRecent(value);
#endif

      m_list.erase(cacheIter->second);
      m_cache.erase(cacheIter);
    }

    const_iterator remove(const_iterator iter) {
      T value = *iter;
#if defined(__AVX2__)
      if constexpr (UseRecentCache)
        removeRecent(value);
#endif

      auto cacheIter = m_cache.find(value);
      m_cache.erase(cacheIter);
      return m_list.erase(iter);
    }

    void touch(const T& value) {
#if defined(__AVX2__)
      if constexpr (UseRecentCache) {
        uint32_t recentIndex = findRecent(value);

        if (recentIndex < m_recentCount) {
          m_list.erase(m_recentIters[recentIndex]);
          m_list.push_back(value);

          auto iter = m_list.cend();
          iter--;

          m_cache[value] = iter;
          removeRecent(value);
          addRecent(value, iter);
          return;
        }
      }
#endif

      auto cacheIter = m_cache.find(value);
      if (cacheIter == m_cache.end())
        return;

      m_list.erase(cacheIter->second);
      m_list.push_back(value);
      auto iter = m_list.cend();
      --iter;
      m_cache[value] = iter;

#if defined(__AVX2__)
      if constexpr (UseRecentCache) {
        removeRecent(value);
        addRecent(value, iter);
      }
#endif
    }

    const_iterator leastRecentlyUsedIter() const {
      return m_list.cbegin();
    }

    const_iterator leastRecentlyUsedEndIter() const {
      return m_list.cend();
    }

    uint32_t size() const noexcept {
      return m_list.size();
    }

  private:
    std::list<T> m_list;
    std::unordered_map<T, const_iterator> m_cache;

#if defined(__AVX2__)
    static constexpr bool UseRecentCache = sizeof(T) == sizeof(uint64_t) && std::is_trivially_copyable_v<T>;

    static uint64_t makeRecentKey(const T& value) {
      uint64_t result = 0u;
      std::memcpy(&result, &value, sizeof(result));
      return result;
    }

    uint32_t findRecent(const T& value) const {
      uint64_t key = makeRecentKey(value);

      __m256i keys = _mm256_load_si256(reinterpret_cast<const __m256i*>(m_recentKeys));
      __m256i needle = _mm256_set1_epi64x(int64_t(key));
      uint32_t mask = uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi64(keys, needle)));

      while (mask) {
        uint32_t index = bit::tzcnt(mask) >> 3u;

        if (index < m_recentCount && *m_recentIters[index] == value)
          return index;

        mask &= ~(0xFFu << (index * 8u));
      }

      return m_recentCount;
    }

    void addRecent(const T& value, const_iterator iter) {
      uint32_t count = std::min<uint32_t>(m_recentCount, 3u);

      for (uint32_t i = count; i; i--) {
        m_recentKeys[i] = m_recentKeys[i - 1u];
        m_recentIters[i] = m_recentIters[i - 1u];
      }

      m_recentKeys[0] = makeRecentKey(value);
      m_recentIters[0] = iter;

      if (m_recentCount < 4u)
        m_recentCount++;
    }

    void removeRecent(const T& value) {
      uint32_t index = findRecent(value);

      if (index >= m_recentCount)
        return;

      for (uint32_t i = index; i + 1u < m_recentCount; i++) {
        m_recentKeys[i] = m_recentKeys[i + 1u];
        m_recentIters[i] = m_recentIters[i + 1u];
      }

      m_recentCount--;
      m_recentKeys[m_recentCount] = 0u;
      m_recentIters[m_recentCount] = const_iterator();
    }

    alignas(32) uint64_t m_recentKeys[4] = {};
    const_iterator m_recentIters[4] = {};
    uint32_t m_recentCount = 0;
#endif

  };

}
