#include "spirv_compression.h"

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace dxvk {

  SpirvCompressedBuffer::SpirvCompressedBuffer()
  : m_size(0) {

  }


  SpirvCompressedBuffer::SpirvCompressedBuffer(SpirvCodeBuffer& code)
  : m_size(code.dwords()) {
    // The compression (detailed below) achieves roughly 55% of the
    // original size on average and is very consistent, so an initial
    // estimate of roughly 58% will be accurate most of the time.
    const uint32_t* data = code.data();
    m_code.reserve((m_size * 75) / 128);

    std::array<uint32_t, 16> block;
    uint32_t blockMask = 0;
    uint32_t blockOffset = 0;

    // The algorithm used is a simple variable-to-fixed compression that
    // encodes up to two consecutive SPIR-V tokens into one DWORD using
    // a small number of different encodings. While not achieving great
    // compression ratios, the main goal is to allow decompression code
    // to be fast, with short dependency chains.
    // Compressed tokens are stored in blocks of 16 DWORDs, each preceeded
    // by a single DWORD which stores the layout for each DWORD, two bits
    // each. The supported layouts, are as follows:
    // 0x0: 1x 32-bit;  0x1: 1x 20-bit + 1x 12-bit
    // 0x2: 2x 16-bit;  0x3: 1x 12-bit + 1x 20-bit
    // These layouts are chosen to allow reasonably efficient encoding of
    // opcode tokens, which usually fit into 20 bits, followed by type IDs,
    // which tend to be low as well since most types are defined early.
    for (size_t i = 0; i < m_size; ) {
      if (likely(i + 1 < m_size)) {
        uint32_t a = data[i];
        uint32_t b = data[i + 1];
        uint32_t schema;
        uint32_t encode;

        if (std::max(a, b) < (1u << 16)) {
          schema = 0x2;
          encode = a | (b << 16);
        } else if (a < (1u << 20) && b < (1u << 12)) {
          schema = 0x1;
          encode = a | (b << 20);
        } else if (a < (1u << 12) && b < (1u << 20)) {
          schema = 0x3;
          encode = a | (b << 12);
        } else {
          schema = 0x0;
          encode = a;
        }

        block[blockOffset] = encode;
        blockMask |= schema << (blockOffset << 1);
        blockOffset += 1;

        i += schema ? 2 : 1;
      } else {
        block[blockOffset] = data[i++];
        blockOffset += 1;
      }

      if (unlikely(blockOffset == 16) || unlikely(i == m_size)) {
        m_code.insert(m_code.end(), blockMask);
        m_code.insert(m_code.end(), block.begin(), block.begin() + blockOffset);

        blockMask = 0;
        blockOffset = 0;
      }
    }

    // Only shrink the array if we have lots of overhead for some reason.
    // This should only happen on shaders where our initial estimate was
    // too small. In general, we want to avoid reallocation here.
    if (m_code.capacity() > (m_code.size() * 10) / 9)
      m_code.shrink_to_fit();
  }

    
  SpirvCompressedBuffer::~SpirvCompressedBuffer() {

  }


  SpirvCodeBuffer SpirvCompressedBuffer::decompress() const {
    SpirvCodeBuffer code(m_size);
    uint32_t* data = code.data();

    uint32_t srcOffset = 0;
    uint32_t dstOffset = 0;

#if defined(__AVX2__)
    const __m256i shift_schema_vec0 = _mm256_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14);
    const __m256i shift_schema_vec1 = _mm256_setr_epi32(16, 18, 20, 22, 24, 26, 28, 30);
    const __m256i mask_table = _mm256_setr_epi32(
      0xffffffff, // schema 0
      0x000fffff, // schema 1
      0x0000ffff, // schema 2
      0x00000fff, // schema 3
      0, 0, 0, 0
    );
    const __m256i shift_table = _mm256_setr_epi32(
      0,  // schema 0
      20, // schema 1
      16, // schema 2
      12, // schema 3
      0, 0, 0, 0
    );
    const __m256i three = _mm256_set1_epi32(3);

    while (srcOffset + 17 <= m_code.size() && dstOffset + 32 <= m_size) {
      uint32_t blockMask = m_code[srcOffset];
      uint32_t schemaBits = blockMask | (blockMask >> 1);

      if ((schemaBits & 0x55555555u) != 0x55555555u)
        break;

      // Part 1: i = 0..7
      {
        __m256i encode_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&m_code[srcOffset + 1]));
        __m256i mask_broad = _mm256_set1_epi32(blockMask);
        __m256i shifted = _mm256_srlv_epi32(mask_broad, shift_schema_vec0);
        __m256i schema_vec = _mm256_and_si256(shifted, three);

        __m256i mask_vec = _mm256_permutevar8x32_epi32(mask_table, schema_vec);
        __m256i shift_vec = _mm256_permutevar8x32_epi32(shift_table, schema_vec);

        __m256i val_a = _mm256_and_si256(encode_vec, mask_vec);
        __m256i val_b = _mm256_srlv_epi32(encode_vec, shift_vec);

        alignas(32) uint32_t val_a_arr[8];
        alignas(32) uint32_t val_b_arr[8];
        _mm256_store_si256(reinterpret_cast<__m256i*>(val_a_arr), val_a);
        _mm256_store_si256(reinterpret_cast<__m256i*>(val_b_arr), val_b);

        for (int i = 0; i < 8; i++) {
          data[dstOffset + 2 * i + 0] = val_a_arr[i];
          data[dstOffset + 2 * i + 1] = val_b_arr[i];
        }
      }

      // Part 2: i = 8..15
      {
        __m256i encode_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&m_code[srcOffset + 9]));
        __m256i mask_broad = _mm256_set1_epi32(blockMask);
        __m256i shifted = _mm256_srlv_epi32(mask_broad, shift_schema_vec1);
        __m256i schema_vec = _mm256_and_si256(shifted, three);

        __m256i mask_vec = _mm256_permutevar8x32_epi32(mask_table, schema_vec);
        __m256i shift_vec = _mm256_permutevar8x32_epi32(shift_table, schema_vec);

        __m256i val_a = _mm256_and_si256(encode_vec, mask_vec);
        __m256i val_b = _mm256_srlv_epi32(encode_vec, shift_vec);

        alignas(32) uint32_t val_a_arr[8];
        alignas(32) uint32_t val_b_arr[8];
        _mm256_store_si256(reinterpret_cast<__m256i*>(val_a_arr), val_a);
        _mm256_store_si256(reinterpret_cast<__m256i*>(val_b_arr), val_b);

        for (int i = 0; i < 8; i++) {
          data[dstOffset + 16 + 2 * i + 0] = val_a_arr[i];
          data[dstOffset + 16 + 2 * i + 1] = val_b_arr[i];
        }
      }

      dstOffset += 32;
      srcOffset += 17;
    }
#endif

    constexpr uint32_t shiftAmounts = 0x0c101420;

    while (dstOffset < m_size) {
      uint32_t blockMask = m_code[srcOffset];

      for (uint32_t i = 0; i < 16 && dstOffset < m_size; i++) {
        // Use 64-bit integers for some of the operands so we can
        // shift by 32 bits and not handle it as a special cases
        uint32_t schema = (blockMask >> (i << 1)) & 0x3;
        uint32_t shift  = (shiftAmounts >> (schema << 3)) & 0xff;
        uint64_t mask   = ~(~0ull << shift);
        uint64_t encode = m_code[srcOffset + i + 1];

        data[dstOffset] = encode & mask;

        if (likely(schema)) {
          if (likely(dstOffset + 1 < m_size)) {
            data[dstOffset + 1] = encode >> shift;
          }
        }

        dstOffset += schema ? 2 : 1;
      }

      srcOffset += 17;
    }

    return code;
  }

}
