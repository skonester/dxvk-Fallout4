#pragma once

#include <iostream>

#include "util_bit.h"
#include "util_math.h"

namespace dxvk {

  template <typename T>
  struct Vector4Base {
    Vector4Base()
      : x{ }, y{ }, z{ }, w{ } { }

    Vector4Base(T splat)
      : x(splat), y(splat), z(splat), w(splat) { }

    Vector4Base(T x, T y, T z, T w)
      : x(x), y(y), z(z), w(w) { }

    Vector4Base(const T xyzw[4])
      : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) { }

    Vector4Base(const Vector4Base<T>& other) = default;
    Vector4Base& operator=(const Vector4Base<T>& other) = default;

    inline       T& operator[](size_t index)       { return data[index]; }
    inline const T& operator[](size_t index) const { return data[index]; }

    bool operator==(const Vector4Base<T>& other) const {
      for (uint32_t i = 0; i < 4; i++) {
        if (data[i] != other.data[i])
        return false;
      }

      return true;
    }

    bool operator!=(const Vector4Base<T>& other) const {
      return !operator==(other);
    }

    Vector4Base operator-() const { return {-x, -y, -z, -w}; }

    Vector4Base operator+(const Vector4Base<T>& other) const {
      return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    Vector4Base operator-(const Vector4Base<T>& other) const {
      return {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    Vector4Base operator*(T scalar) const {
      return {scalar * x, scalar * y, scalar * z, scalar * w};
    }

    Vector4Base operator*(const Vector4Base<T>& other) const {
      Vector4Base result;
      for (uint32_t i = 0; i < 4; i++)
        result[i] = data[i] * other.data[i];
      return result;
    }

    Vector4Base operator/(const Vector4Base<T>& other) const {
      Vector4Base result;
      for (uint32_t i = 0; i < 4; i++)
        result[i] = data[i] / other.data[i];
      return result;
    }

    Vector4Base operator/(T scalar) const {
      return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    Vector4Base& operator+=(const Vector4Base<T>& other) {
      x += other.x;
      y += other.y;
      z += other.z;
      w += other.w;

      return *this;
    }

    Vector4Base& operator-=(const Vector4Base<T>& other) {
      x -= other.x;
      y -= other.y;
      z -= other.z;
      w -= other.w;

      return *this;
    }

    Vector4Base& operator*=(T scalar) {
      x *= scalar;
      y *= scalar;
      z *= scalar;
      w *= scalar;

      return *this;
    }

    Vector4Base& operator/=(T scalar) {
      x /= scalar;
      y /= scalar;
      z /= scalar;
      w /= scalar;

      return *this;
    }

    union {
      T data[4];
      struct {
        T x, y, z, w;
      };
      struct {
        T r, g, b, a;
      };
    };

  };

  template <>
  inline Vector4Base<float> Vector4Base<float>::operator+(const Vector4Base<float>& other) const {
    #ifdef DXVK_ARCH_X86
    Vector4Base<float> result;
    _mm_storeu_ps(result.data, _mm_add_ps(_mm_loadu_ps(data), _mm_loadu_ps(other.data)));
    return result;
    #else
    return {x + other.x, y + other.y, z + other.z, w + other.w};
    #endif
  }

  template <>
  inline Vector4Base<float> Vector4Base<float>::operator-(const Vector4Base<float>& other) const {
    #ifdef DXVK_ARCH_X86
    Vector4Base<float> result;
    _mm_storeu_ps(result.data, _mm_sub_ps(_mm_loadu_ps(data), _mm_loadu_ps(other.data)));
    return result;
    #else
    return {x - other.x, y - other.y, z - other.z, w - other.w};
    #endif
  }

  template <>
  inline Vector4Base<float> Vector4Base<float>::operator*(float scalar) const {
    #ifdef DXVK_ARCH_X86
    Vector4Base<float> result;
    _mm_storeu_ps(result.data, _mm_mul_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
    return result;
    #else
    return {scalar * x, scalar * y, scalar * z, scalar * w};
    #endif
  }

  template <>
  inline Vector4Base<float> Vector4Base<float>::operator*(const Vector4Base<float>& other) const {
    #ifdef DXVK_ARCH_X86
    Vector4Base<float> result;
    _mm_storeu_ps(result.data, _mm_mul_ps(_mm_loadu_ps(data), _mm_loadu_ps(other.data)));
    return result;
    #else
    Vector4Base<float> result;
    for (uint32_t i = 0; i < 4; i++)
      result[i] = data[i] * other.data[i];
    return result;
    #endif
  }

  template <>
  inline Vector4Base<float>& Vector4Base<float>::operator+=(const Vector4Base<float>& other) {
    #ifdef DXVK_ARCH_X86
    _mm_storeu_ps(data, _mm_add_ps(_mm_loadu_ps(data), _mm_loadu_ps(other.data)));
    return *this;
    #else
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
    #endif
  }

  template <>
  inline Vector4Base<float>& Vector4Base<float>::operator-=(const Vector4Base<float>& other) {
    #ifdef DXVK_ARCH_X86
    _mm_storeu_ps(data, _mm_sub_ps(_mm_loadu_ps(data), _mm_loadu_ps(other.data)));
    return *this;
    #else
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
    #endif
  }

  template <>
  inline Vector4Base<float>& Vector4Base<float>::operator*=(float scalar) {
    #ifdef DXVK_ARCH_X86
    _mm_storeu_ps(data, _mm_mul_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
    return *this;
    #else
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
    #endif
  }

  template <typename T>
  inline Vector4Base<T> operator*(T scalar, const Vector4Base<T>& vector) {
    return vector * scalar;
  }

  template <typename T>
  float dot(const Vector4Base<T>& a, const Vector4Base<T>& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  }

  template <typename T>
  T lengthSqr(const Vector4Base<T>& a) { return dot(a, a); }

  template <typename T>
  float length(const Vector4Base<T>& a) { return std::sqrt(float(lengthSqr(a))); }

  template <typename T>
  Vector4Base<T> normalize(const Vector4Base<T>& a) { return a * T(1.0f / length(a)); }

  template <typename T>
  std::ostream& operator<<(std::ostream& os, const Vector4Base<T>& v) {
    return os << "Vector4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }

  using Vector4  = Vector4Base<float>;
  using Vector4i = Vector4Base<int>;

  static_assert(sizeof(Vector4)  == sizeof(float) * 4);
  static_assert(sizeof(Vector4i) == sizeof(int)   * 4);

  #if defined(__AVX2__)
  struct Vector4x2 {
    Vector4x2()
    : value(_mm256_setzero_ps()) {

    }

    explicit Vector4x2(__m256 value)
    : value(value) {

    }

    Vector4x2(const Vector4& lo, const Vector4& hi)
    : value(_mm256_set_m128(_mm_loadu_ps(hi.data), _mm_loadu_ps(lo.data))) {

    }

    static Vector4x2 load(const Vector4* vectors) {
      return Vector4x2(_mm256_loadu_ps(reinterpret_cast<const float*>(vectors)));
    }

    void store(Vector4* vectors) const {
      _mm256_storeu_ps(reinterpret_cast<float*>(vectors), value);
    }

    Vector4 lo() const {
      Vector4 result;
      _mm_storeu_ps(result.data, _mm256_castps256_ps128(value));
      return result;
    }

    Vector4 hi() const {
      Vector4 result;
      _mm_storeu_ps(result.data, _mm256_extractf128_ps(value, 1));
      return result;
    }

    Vector4x2 operator+(const Vector4x2& other) const {
      return Vector4x2(_mm256_add_ps(value, other.value));
    }

    Vector4x2 operator-(const Vector4x2& other) const {
      return Vector4x2(_mm256_sub_ps(value, other.value));
    }

    Vector4x2 operator*(const Vector4x2& other) const {
      return Vector4x2(_mm256_mul_ps(value, other.value));
    }

    Vector4x2 operator*(float scalar) const {
      return Vector4x2(_mm256_mul_ps(value, _mm256_set1_ps(scalar)));
    }

    Vector4x2& operator+=(const Vector4x2& other) {
      value = _mm256_add_ps(value, other.value);
      return *this;
    }

    Vector4x2& operator-=(const Vector4x2& other) {
      value = _mm256_sub_ps(value, other.value);
      return *this;
    }

    Vector4x2& operator*=(float scalar) {
      value = _mm256_mul_ps(value, _mm256_set1_ps(scalar));
      return *this;
    }

    __m256 value;
  };

  inline Vector4x2 operator*(float scalar, const Vector4x2& vector) {
    return vector * scalar;
  }
  #endif

  inline Vector4 replaceNaN(Vector4 a) {
    #ifdef DXVK_ARCH_X86
    Vector4 result;
    __m128 value = _mm_loadu_ps(a.data);
    __m128 mask  = _mm_cmpeq_ps(value, value);
           value = _mm_and_ps(value, mask);
    _mm_storeu_ps(result.data, value);
    return result;
    #else
    for (int i = 0; i < 4; i++)
      a[i] = std::isnan(a[i]) ? 0.0f : a[i];
    return a;
    #endif
  }

}
