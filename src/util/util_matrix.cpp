#include "util_matrix.h"
#include "util_simd_perf.h"

#if defined(__AVX2__) || defined(__FMA__)
  #include <immintrin.h>
#endif

namespace dxvk {

        Vector4& Matrix4::operator[](size_t index)       { return data[index]; }
  const Vector4& Matrix4::operator[](size_t index) const { return data[index]; }

  bool Matrix4::operator==(const Matrix4& m2) const {
    const Matrix4& m1 = *this;
    for (uint32_t i = 0; i < 4; i++) {
      if (m1[i] != m2[i])
        return false;
    }
    return true;
  }

  bool Matrix4::operator!=(const Matrix4& m2) const { return !operator==(m2); }

  Matrix4 Matrix4::operator+(const Matrix4& other) const {
    #if defined(__AVX2__)
    Matrix4 mat;
    auto a = reinterpret_cast<const float*>(data);
    auto b = reinterpret_cast<const float*>(other.data);
    auto r = reinterpret_cast<float*>(mat.data);

    _mm256_storeu_ps(r + 0u, _mm256_add_ps(_mm256_loadu_ps(a + 0u), _mm256_loadu_ps(b + 0u)));
    _mm256_storeu_ps(r + 8u, _mm256_add_ps(_mm256_loadu_ps(a + 8u), _mm256_loadu_ps(b + 8u)));
    return mat;
    #else
    Matrix4 mat;
    for (uint32_t i = 0; i < 4; i++)
      mat[i] = data[i] + other.data[i];
    return mat;
    #endif
  }

  Matrix4 Matrix4::operator-(const Matrix4& other) const {
    #if defined(__AVX2__)
    Matrix4 mat;
    auto a = reinterpret_cast<const float*>(data);
    auto b = reinterpret_cast<const float*>(other.data);
    auto r = reinterpret_cast<float*>(mat.data);

    _mm256_storeu_ps(r + 0u, _mm256_sub_ps(_mm256_loadu_ps(a + 0u), _mm256_loadu_ps(b + 0u)));
    _mm256_storeu_ps(r + 8u, _mm256_sub_ps(_mm256_loadu_ps(a + 8u), _mm256_loadu_ps(b + 8u)));
    return mat;
    #else
    Matrix4 mat;
    for (uint32_t i = 0; i < 4; i++)
      mat[i] = data[i] - other.data[i];
    return mat;
    #endif
  }

  Matrix4 Matrix4::operator*(const Matrix4& m2) const {
    const Matrix4& m1 = *this;

    #if defined(__FMA__)
    const __m128 a0 = _mm_loadu_ps(m1[0].data);
    const __m128 a1 = _mm_loadu_ps(m1[1].data);
    const __m128 a2 = _mm_loadu_ps(m1[2].data);
    const __m128 a3 = _mm_loadu_ps(m1[3].data);

    Matrix4 result;

    for (uint32_t i = 0; i < 4; i++) {
      __m128 r = _mm_mul_ps(a0, _mm_set1_ps(m2[i][0]));
      r = _mm_fmadd_ps(a1, _mm_set1_ps(m2[i][1]), r);
      r = _mm_fmadd_ps(a2, _mm_set1_ps(m2[i][2]), r);
      r = _mm_fmadd_ps(a3, _mm_set1_ps(m2[i][3]), r);
      _mm_storeu_ps(result[i].data, r);
    }

    return result;
    #else
    const Vector4 srcA0 = { m1[0] };
    const Vector4 srcA1 = { m1[1] };
    const Vector4 srcA2 = { m1[2] };
    const Vector4 srcA3 = { m1[3] };

    const Vector4 srcB0 = { m2[0] };
    const Vector4 srcB1 = { m2[1] };
    const Vector4 srcB2 = { m2[2] };
    const Vector4 srcB3 = { m2[3] };

    Matrix4 result;
    result[0] = srcA0 * srcB0[0] + srcA1 * srcB0[1] + srcA2 * srcB0[2] + srcA3 * srcB0[3];
    result[1] = srcA0 * srcB1[0] + srcA1 * srcB1[1] + srcA2 * srcB1[2] + srcA3 * srcB1[3];
    result[2] = srcA0 * srcB2[0] + srcA1 * srcB2[1] + srcA2 * srcB2[2] + srcA3 * srcB2[3];
    result[3] = srcA0 * srcB3[0] + srcA1 * srcB3[1] + srcA2 * srcB3[2] + srcA3 * srcB3[3];
    return result;
    #endif
  }

  Vector4 Matrix4::operator*(const Vector4& v) const {
    const Matrix4& m = *this;

    #if defined(__FMA__)
    __m128 r = _mm_mul_ps(_mm_loadu_ps(m[0].data), _mm_set1_ps(v[0]));
    r = _mm_fmadd_ps(_mm_loadu_ps(m[1].data), _mm_set1_ps(v[1]), r);
    r = _mm_fmadd_ps(_mm_loadu_ps(m[2].data), _mm_set1_ps(v[2]), r);
    r = _mm_fmadd_ps(_mm_loadu_ps(m[3].data), _mm_set1_ps(v[3]), r);

    Vector4 result;
    _mm_storeu_ps(result.data, r);
    return result;
    #else
    const Vector4 mul0 = { m[0] * v[0] };
    const Vector4 mul1 = { m[1] * v[1] };
    const Vector4 mul2 = { m[2] * v[2] };
    const Vector4 mul3 = { m[3] * v[3] };

    const Vector4 add0 = { mul0 + mul1 };
    const Vector4 add1 = { mul2 + mul3 };

    return add0 + add1;
    #endif
  }

  Matrix4 Matrix4::operator*(float scalar) const {
    #if defined(__AVX2__)
    Matrix4 mat;
    auto a = reinterpret_cast<const float*>(data);
    auto r = reinterpret_cast<float*>(mat.data);
    auto s = _mm256_set1_ps(scalar);

    _mm256_storeu_ps(r + 0u, _mm256_mul_ps(_mm256_loadu_ps(a + 0u), s));
    _mm256_storeu_ps(r + 8u, _mm256_mul_ps(_mm256_loadu_ps(a + 8u), s));
    return mat;
    #else
    Matrix4 mat;
    for (uint32_t i = 0; i < 4; i++)
      mat[i] = data[i] * scalar;
    return mat;
    #endif
  }

  Matrix4 Matrix4::operator/(float scalar) const {
    #if defined(__AVX2__)
    Matrix4 mat;
    auto a = reinterpret_cast<const float*>(data);
    auto r = reinterpret_cast<float*>(mat.data);
    auto s = _mm256_set1_ps(scalar);

    _mm256_storeu_ps(r + 0u, _mm256_div_ps(_mm256_loadu_ps(a + 0u), s));
    _mm256_storeu_ps(r + 8u, _mm256_div_ps(_mm256_loadu_ps(a + 8u), s));
    return mat;
    #else
    Matrix4 mat;
    for (uint32_t i = 0; i < 4; i++)
      mat[i] = data[i] / scalar;
    return mat;
    #endif
  }

  Matrix4& Matrix4::operator+=(const Matrix4& other) {
    #if defined(__AVX2__)
    auto a = reinterpret_cast<float*>(data);
    auto b = reinterpret_cast<const float*>(other.data);

    _mm256_storeu_ps(a + 0u, _mm256_add_ps(_mm256_loadu_ps(a + 0u), _mm256_loadu_ps(b + 0u)));
    _mm256_storeu_ps(a + 8u, _mm256_add_ps(_mm256_loadu_ps(a + 8u), _mm256_loadu_ps(b + 8u)));
    #else
    for (uint32_t i = 0; i < 4; i++)
      data[i] += other.data[i];
    #endif
    return *this;
  }

  Matrix4& Matrix4::operator-=(const Matrix4& other) {
    #if defined(__AVX2__)
    auto a = reinterpret_cast<float*>(data);
    auto b = reinterpret_cast<const float*>(other.data);

    _mm256_storeu_ps(a + 0u, _mm256_sub_ps(_mm256_loadu_ps(a + 0u), _mm256_loadu_ps(b + 0u)));
    _mm256_storeu_ps(a + 8u, _mm256_sub_ps(_mm256_loadu_ps(a + 8u), _mm256_loadu_ps(b + 8u)));
    #else
    for (uint32_t i = 0; i < 4; i++)
      data[i] -= other.data[i];
    #endif
    return *this;
  }

  Matrix4& Matrix4::operator*=(const Matrix4& other) {
    return (*this = (*this) * other);
  }

  Matrix4 transpose(const Matrix4& m) {
    #if defined(__AVX2__)
    DXVK_SIMD_PERF_SCOPE(MatrixOps);
    __m256 m01 = _mm256_loadu_ps(m[0].data);
    __m256 m23 = _mm256_loadu_ps(m[2].data);

    __m256 t0 = _mm256_shuffle_ps(m01, m23, _MM_SHUFFLE(1, 0, 1, 0));
    __m256 t1 = _mm256_shuffle_ps(m01, m23, _MM_SHUFFLE(3, 2, 3, 2));

    const __m256i perm_idx = _mm256_setr_epi32(0, 4, 2, 6, 1, 5, 3, 7);
    __m256 trans01 = _mm256_permutevar8x32_ps(t0, perm_idx);
    __m256 trans23 = _mm256_permutevar8x32_ps(t1, perm_idx);

    Matrix4 result;
    _mm256_storeu_ps(result[0].data, trans01);
    _mm256_storeu_ps(result[2].data, trans23);
    return result;
    #else
    Matrix4 result;

    for (uint32_t i = 0; i < 4; i++) {
      for (uint32_t j = 0; j < 4; j++)
        result[i][j] = m.data[j][i];
    }
    return result;
    #endif
  }

  float determinant(const Matrix4& m) {
    #if defined(__AVX2__)
    DXVK_SIMD_PERF_SCOPE(MatrixOps);
    const __m256 a = _mm256_setr_ps(m[2][2], m[2][1], m[2][1], m[2][0], m[2][0], m[2][0], 0.0f, 0.0f);
    const __m256 b = _mm256_setr_ps(m[3][3], m[3][3], m[3][2], m[3][3], m[3][2], m[3][1], 0.0f, 0.0f);
    const __m256 c = _mm256_setr_ps(m[3][2], m[3][1], m[3][1], m[3][0], m[3][0], m[3][0], 0.0f, 0.0f);
    const __m256 d = _mm256_setr_ps(m[2][3], m[2][3], m[2][2], m[2][3], m[2][2], m[2][1], 0.0f, 0.0f);

    alignas(32) float coef[8];
    _mm256_store_ps(coef, _mm256_sub_ps(_mm256_mul_ps(a, b), _mm256_mul_ps(c, d)));

    const float coef00 = coef[0];
    const float coef04 = coef[1];
    const float coef08 = coef[2];
    const float coef12 = coef[3];
    const float coef16 = coef[4];
    const float coef20 = coef[5];

    const float cofactor0 =  m[1][1] * coef00 - m[1][2] * coef04 + m[1][3] * coef08;
    const float cofactor1 = -m[1][0] * coef00 + m[1][2] * coef12 - m[1][3] * coef16;
    const float cofactor2 =  m[1][0] * coef04 - m[1][1] * coef12 + m[1][3] * coef20;
    const float cofactor3 = -m[1][0] * coef08 + m[1][1] * coef16 - m[1][2] * coef20;

    return ((m[0][0] * cofactor0 + m[0][1] * cofactor1) + (m[0][2] * cofactor2 + m[0][3] * cofactor3));
    #else
    float coef00    =  m[2][2] * m[3][3] - m[3][2] * m[2][3];
    float coef02    =  m[1][2] * m[3][3] - m[3][2] * m[1][3];
    float coef03    =  m[1][2] * m[2][3] - m[2][2] * m[1][3];

    float coef04    =  m[2][1] * m[3][3] - m[3][1] * m[2][3];
    float coef06    =  m[1][1] * m[3][3] - m[3][1] * m[1][3];
    float coef07    =  m[1][1] * m[2][3] - m[2][1] * m[1][3];

    float coef08    =  m[2][1] * m[3][2] - m[3][1] * m[2][2];
    float coef10    =  m[1][1] * m[3][2] - m[3][1] * m[1][2];
    float coef11    =  m[1][1] * m[2][2] - m[2][1] * m[1][2];

    float coef12    =  m[2][0] * m[3][3] - m[3][0] * m[2][3];
    float coef14    =  m[1][0] * m[3][3] - m[3][0] * m[1][3];
    float coef15    =  m[1][0] * m[2][3] - m[2][0] * m[1][3];

    float coef16    =  m[2][0] * m[3][2] - m[3][0] * m[2][2];
    float coef18    =  m[1][0] * m[3][2] - m[3][0] * m[1][2];
    float coef19    =  m[1][0] * m[2][2] - m[2][0] * m[1][2];

    float coef20    =  m[2][0] * m[3][1] - m[3][0] * m[2][1];
    float coef22    =  m[1][0] * m[3][1] - m[3][0] * m[1][1];
    float coef23    =  m[1][0] * m[2][1] - m[2][0] * m[1][1];

    Vector4 fac0    = { coef00, coef00, coef02, coef03 };
    Vector4 fac1    = { coef04, coef04, coef06, coef07 };
    Vector4 fac2    = { coef08, coef08, coef10, coef11 };
    Vector4 fac3    = { coef12, coef12, coef14, coef15 };
    Vector4 fac4    = { coef16, coef16, coef18, coef19 };
    Vector4 fac5    = { coef20, coef20, coef22, coef23 };

    Vector4 vec0    = { m[1][0], m[0][0], m[0][0], m[0][0] };
    Vector4 vec1    = { m[1][1], m[0][1], m[0][1], m[0][1] };
    Vector4 vec2    = { m[1][2], m[0][2], m[0][2], m[0][2] };
    Vector4 vec3    = { m[1][3], m[0][3], m[0][3], m[0][3] };

    Vector4 inv0    = { vec1 * fac0 - vec2 * fac1 + vec3 * fac2 };
    Vector4 inv1    = { vec0 * fac0 - vec2 * fac3 + vec3 * fac4 };
    Vector4 inv2    = { vec0 * fac1 - vec1 * fac3 + vec3 * fac5 };
    Vector4 inv3    = { vec0 * fac2 - vec1 * fac4 + vec2 * fac5 };

    Vector4 signA   = { +1, -1, +1, -1 };
    Vector4 signB   = { -1, +1, -1, +1 };
    Matrix4 inverse = { inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB };

    Vector4 row0    = { inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0] };

    Vector4 dot0    = { m[0] * row0 };

    return (dot0.x + dot0.y) + (dot0.z + dot0.w);
    #endif
  }

  Matrix4 inverse(const Matrix4& m)
  {
    #if defined(__AVX2__)
    DXVK_SIMD_PERF_SCOPE(MatrixOps);
    // We define the elements for L0, R0, L1, R1 for the 3 groups
    // Group 0: fac0 and fac1
    const __m256 L0_01 = _mm256_setr_ps(
      m[2][2], m[2][2], m[1][2], m[1][2], // fac0
      m[2][1], m[2][1], m[1][1], m[1][1]  // fac1
    );
    const __m256 R0_01 = _mm256_setr_ps(
      m[3][3], m[3][3], m[3][3], m[2][3], // fac0
      m[3][3], m[3][3], m[3][3], m[2][3]  // fac1
    );
    const __m256 L1_01 = _mm256_setr_ps(
      m[3][2], m[3][2], m[3][2], m[2][2], // fac0
      m[3][1], m[3][1], m[3][1], m[2][1]  // fac1
    );
    const __m256 R1_01 = _mm256_setr_ps(
      m[2][3], m[2][3], m[1][3], m[1][3], // fac0
      m[2][3], m[2][3], m[1][3], m[1][3]  // fac1
    );

    // Group 1: fac2 and fac3
    const __m256 L0_23 = _mm256_setr_ps(
      m[2][1], m[2][1], m[1][1], m[1][1], // fac2
      m[2][0], m[2][0], m[1][0], m[1][0]  // fac3
    );
    const __m256 R0_23 = _mm256_setr_ps(
      m[3][2], m[3][2], m[3][2], m[2][2], // fac2
      m[3][3], m[3][3], m[3][3], m[2][3]  // fac3
    );
    const __m256 L1_23 = _mm256_setr_ps(
      m[3][1], m[3][1], m[3][1], m[2][1], // fac2
      m[3][0], m[3][0], m[3][0], m[2][0]  // fac3
    );
    const __m256 R1_23 = _mm256_setr_ps(
      m[2][2], m[2][2], m[1][2], m[1][2], // fac2
      m[2][3], m[2][3], m[1][3], m[1][3]  // fac3
    );

    // Group 2: fac4 and fac5
    const __m256 L0_45 = _mm256_setr_ps(
      m[2][0], m[2][0], m[1][0], m[1][0], // fac4
      m[2][0], m[2][0], m[1][0], m[1][0]  // fac5
    );
    const __m256 R0_45 = _mm256_setr_ps(
      m[3][2], m[3][2], m[3][2], m[2][2], // fac4
      m[3][1], m[3][1], m[3][1], m[2][1]  // fac5
    );
    const __m256 L1_45 = _mm256_setr_ps(
      m[3][0], m[3][0], m[3][0], m[2][0], // fac4
      m[3][0], m[3][0], m[3][0], m[2][0]  // fac5
    );
    const __m256 R1_45 = _mm256_setr_ps(
      m[2][2], m[2][2], m[1][2], m[1][2], // fac4
      m[2][1], m[2][1], m[1][1], m[1][1]  // fac5
    );

    // Compute fac01, fac23, fac45
    __m256 fac01 = _mm256_sub_ps(_mm256_mul_ps(L0_01, R0_01), _mm256_mul_ps(L1_01, R1_01));
    __m256 fac23 = _mm256_sub_ps(_mm256_mul_ps(L0_23, R0_23), _mm256_mul_ps(L1_23, R1_23));
    __m256 fac45 = _mm256_sub_ps(_mm256_mul_ps(L0_45, R0_45), _mm256_mul_ps(L1_45, R1_45));

    // Extract fac0 to fac5 as __m128
    __m128 fac0 = _mm256_castps256_ps128(fac01);
    __m128 fac1 = _mm256_extractf128_ps(fac01, 1);
    __m128 fac2 = _mm256_castps256_ps128(fac23);
    __m128 fac3 = _mm256_extractf128_ps(fac23, 1);
    __m128 fac4 = _mm256_castps256_ps128(fac45);
    __m128 fac5 = _mm256_extractf128_ps(fac45, 1);

    // Load vec0 to vec3
    __m128 vec0 = _mm_setr_ps(m[1][0], m[0][0], m[0][0], m[0][0]);
    __m128 vec1 = _mm_setr_ps(m[1][1], m[0][1], m[0][1], m[0][1]);
    __m128 vec2 = _mm_setr_ps(m[1][2], m[0][2], m[0][2], m[0][2]);
    __m128 vec3 = _mm_setr_ps(m[1][3], m[0][3], m[0][3], m[0][3]);

    // Compute inv0 to inv3
    #if defined(__FMA__)
    __m128 inv0 = _mm_fmadd_ps(vec3, fac2, _mm_fmsub_ps(vec1, fac0, _mm_mul_ps(vec2, fac1)));
    __m128 inv1 = _mm_fmadd_ps(vec3, fac4, _mm_fmsub_ps(vec0, fac0, _mm_mul_ps(vec2, fac3)));
    __m128 inv2 = _mm_fmadd_ps(vec3, fac5, _mm_fmsub_ps(vec0, fac1, _mm_mul_ps(vec1, fac3)));
    __m128 inv3 = _mm_fmadd_ps(vec2, fac5, _mm_fmsub_ps(vec0, fac2, _mm_mul_ps(vec1, fac4)));
    #else
    __m128 inv0 = _mm_add_ps(_mm_sub_ps(_mm_mul_ps(vec1, fac0), _mm_mul_ps(vec2, fac1)), _mm_mul_ps(vec3, fac2));
    __m128 inv1 = _mm_add_ps(_mm_sub_ps(_mm_mul_ps(vec0, fac0), _mm_mul_ps(vec2, fac3)), _mm_mul_ps(vec3, fac4));
    __m128 inv2 = _mm_add_ps(_mm_sub_ps(_mm_mul_ps(vec0, fac1), _mm_mul_ps(vec1, fac3)), _mm_mul_ps(vec3, fac5));
    __m128 inv3 = _mm_add_ps(_mm_sub_ps(_mm_mul_ps(vec0, fac2), _mm_mul_ps(vec1, fac4)), _mm_mul_ps(vec2, fac5));
    #endif

    // Sign vectors
    const __m128 signA = _mm_setr_ps(1.0f, -1.0f, 1.0f, -1.0f);
    const __m128 signB = _mm_setr_ps(-1.0f, 1.0f, -1.0f, 1.0f);

    inv0 = _mm_mul_ps(inv0, signA);
    inv1 = _mm_mul_ps(inv1, signB);
    inv2 = _mm_mul_ps(inv2, signA);
    inv3 = _mm_mul_ps(inv3, signB);

    // Compute determinant
    __m128 row0 = _mm_setr_ps(
      _mm_cvtss_f32(inv0),
      _mm_cvtss_f32(inv1),
      _mm_cvtss_f32(inv2),
      _mm_cvtss_f32(inv3)
    );
    __m128 dot0 = _mm_mul_ps(_mm_loadu_ps(m[0].data), row0);
    __m128 shuf = _mm_movehdup_ps(dot0);
    __m128 sums = _mm_add_ps(dot0, shuf);
    sums = _mm_add_ss(sums, _mm_movehl_ps(sums, sums));
    float dot1 = _mm_cvtss_f32(sums);

    if (unlikely(std::abs(dot1) <= std::numeric_limits<float>::min() * 10)) {
      return m;
    }

    float invDet = 1.0f / dot1;
    __m128 vInvDet = _mm_set1_ps(invDet);

    Matrix4 result;
    _mm_storeu_ps(result[0].data, _mm_mul_ps(inv0, vInvDet));
    _mm_storeu_ps(result[1].data, _mm_mul_ps(inv1, vInvDet));
    _mm_storeu_ps(result[2].data, _mm_mul_ps(inv2, vInvDet));
    _mm_storeu_ps(result[3].data, _mm_mul_ps(inv3, vInvDet));
    return result;
    #else
    float coef00    = m[2][2] * m[3][3] - m[3][2] * m[2][3];
    float coef02    = m[1][2] * m[3][3] - m[3][2] * m[1][3];
    float coef03    = m[1][2] * m[2][3] - m[2][2] * m[1][3];
    float coef04    = m[2][1] * m[3][3] - m[3][1] * m[2][3];
    float coef06    = m[1][1] * m[3][3] - m[3][1] * m[1][3];
    float coef07    = m[1][1] * m[2][3] - m[2][1] * m[1][3];
    float coef08    = m[2][1] * m[3][2] - m[3][1] * m[2][2];
    float coef10    = m[1][1] * m[3][2] - m[3][1] * m[1][2];
    float coef11    = m[1][1] * m[2][2] - m[2][1] * m[1][2];
    float coef12    = m[2][0] * m[3][3] - m[3][0] * m[2][3];
    float coef14    = m[1][0] * m[3][3] - m[3][0] * m[1][3];
    float coef15    = m[1][0] * m[2][3] - m[2][0] * m[1][3];
    float coef16    = m[2][0] * m[3][2] - m[3][0] * m[2][2];
    float coef18    = m[1][0] * m[3][2] - m[3][0] * m[1][2];
    float coef19    = m[1][0] * m[2][2] - m[2][0] * m[1][2];
    float coef20    = m[2][0] * m[3][1] - m[3][0] * m[2][1];
    float coef22    = m[1][0] * m[3][1] - m[3][0] * m[1][1];
    float coef23    = m[1][0] * m[2][1] - m[2][0] * m[1][1];
  
    Vector4 fac0    = { coef00, coef00, coef02, coef03 };
    Vector4 fac1    = { coef04, coef04, coef06, coef07 };
    Vector4 fac2    = { coef08, coef08, coef10, coef11 };
    Vector4 fac3    = { coef12, coef12, coef14, coef15 };
    Vector4 fac4    = { coef16, coef16, coef18, coef19 };
    Vector4 fac5    = { coef20, coef20, coef22, coef23 };
  
    Vector4 vec0    = { m[1][0], m[0][0], m[0][0], m[0][0] };
    Vector4 vec1    = { m[1][1], m[0][1], m[0][1], m[0][1] };
    Vector4 vec2    = { m[1][2], m[0][2], m[0][2], m[0][2] };
    Vector4 vec3    = { m[1][3], m[0][3], m[0][3], m[0][3] };
  
    Vector4 inv0    = { vec1 * fac0 - vec2 * fac1 + vec3 * fac2 };
    Vector4 inv1    = { vec0 * fac0 - vec2 * fac3 + vec3 * fac4 };
    Vector4 inv2    = { vec0 * fac1 - vec1 * fac3 + vec3 * fac5 };
    Vector4 inv3    = { vec0 * fac2 - vec1 * fac4 + vec2 * fac5 };
  
    Vector4 signA   = { +1, -1, +1, -1 };
    Vector4 signB   = { -1, +1, -1, +1 };
    Matrix4 inverse = { inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB };

    Vector4 row0    = { inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0] };

    Vector4 dot0    = { m[0] * row0 };
    float dot1      = (dot0.x + dot0.y) + (dot0.z + dot0.w);

    if (unlikely(std::abs(dot1) <= std::numeric_limits<float>::min() * 10)) {
      return m;
    }

    return inverse * (1.0f / dot1);
    #endif
  }

  Matrix4 hadamardProduct(const Matrix4& a, const Matrix4& b) {
    #if defined(__AVX2__)
    Matrix4 result;
    auto ap = reinterpret_cast<const float*>(a.data);
    auto bp = reinterpret_cast<const float*>(b.data);
    auto rp = reinterpret_cast<float*>(result.data);

    _mm256_storeu_ps(rp + 0u, _mm256_mul_ps(_mm256_loadu_ps(ap + 0u), _mm256_loadu_ps(bp + 0u)));
    _mm256_storeu_ps(rp + 8u, _mm256_mul_ps(_mm256_loadu_ps(ap + 8u), _mm256_loadu_ps(bp + 8u)));
    return result;
    #else
    Matrix4 result;

    for (uint32_t i = 0; i < 4; i++)
      result[i] = a[i] * b[i];

    return result;
    #endif
  }

  std::ostream& operator<<(std::ostream& os, const Matrix4& m) {
    os << "Matrix4(";
    for (uint32_t i = 0; i < 4; i++) {
      os << "\n\t" << m[i];
      if (i < 3)
        os << ", ";
    }
    os << "\n)";

    return os;
  }

}
