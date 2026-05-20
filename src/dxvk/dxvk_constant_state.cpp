#include "dxvk_constant_state.h"

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace dxvk {

  namespace {

    struct DxvkBlendNormalizeFlags {
      bool colorNoOp;
      bool alphaNoOp;
      bool colorPassThrough;
      bool alphaPassThrough;
    };

    DxvkBlendNormalizeFlags getBlendNormalizeFlags(
            VkBlendFactor colorSrcFactor,
            VkBlendFactor colorDstFactor,
            VkBlendOp     colorBlendOp,
            VkBlendFactor alphaSrcFactor,
            VkBlendFactor alphaDstFactor,
            VkBlendOp     alphaBlendOp) {
#if defined(__AVX2__)
      const __m256i ops = _mm256_setr_epi32(
        int32_t(colorSrcFactor), int32_t(colorDstFactor), int32_t(colorBlendOp), 0,
        int32_t(alphaSrcFactor), int32_t(alphaDstFactor), int32_t(alphaBlendOp), 0);

      const __m256i noOp = _mm256_setr_epi32(
        VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, 0,
        VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, 0);

      const __m256i passThrough = _mm256_setr_epi32(
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 0,
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 0);

      uint32_t noOpMask = uint32_t(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(ops, noOp))));
      uint32_t passThroughMask = uint32_t(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(ops, passThrough))));

      return DxvkBlendNormalizeFlags {
        (noOpMask & 0x7u) == 0x7u,
        (noOpMask & 0x70u) == 0x70u,
        (passThroughMask & 0x7u) == 0x7u,
        (passThroughMask & 0x70u) == 0x70u,
      };
#else
      return DxvkBlendNormalizeFlags {
        colorBlendOp == VK_BLEND_OP_ADD
          && colorSrcFactor == VK_BLEND_FACTOR_ZERO
          && colorDstFactor == VK_BLEND_FACTOR_ONE,
        alphaBlendOp == VK_BLEND_OP_ADD
          && alphaSrcFactor == VK_BLEND_FACTOR_ZERO
          && alphaDstFactor == VK_BLEND_FACTOR_ONE,
        colorBlendOp == VK_BLEND_OP_ADD
          && colorSrcFactor == VK_BLEND_FACTOR_ONE
          && colorDstFactor == VK_BLEND_FACTOR_ZERO,
        alphaBlendOp == VK_BLEND_OP_ADD
          && alphaSrcFactor == VK_BLEND_FACTOR_ONE
          && alphaDstFactor == VK_BLEND_FACTOR_ZERO,
      };
#endif
    }

  }

  bool DxvkStencilOp::normalize(VkCompareOp depthOp) {
    if (writeMask()) {
      // If the depth test always passes, this is irrelevant
      if (depthOp == VK_COMPARE_OP_ALWAYS)
        setDepthFailOp(VK_STENCIL_OP_KEEP);

      // Also mask out unused ops if the stencil test
      // always pases or always fails
      if (compareOp() == VK_COMPARE_OP_ALWAYS)
        setFailOp(VK_STENCIL_OP_KEEP);
      else if (compareOp() == VK_COMPARE_OP_NEVER)
        setPassOp(VK_STENCIL_OP_KEEP);

      // If all stencil ops are no-ops, clear write mask
      if (passOp() == VK_STENCIL_OP_KEEP
       && failOp() == VK_STENCIL_OP_KEEP
       && depthFailOp() == VK_STENCIL_OP_KEEP)
        setWriteMask(0u);
    } else {
      // Normalize stencil ops if write mask is 0
      setPassOp(VK_STENCIL_OP_KEEP);
      setFailOp(VK_STENCIL_OP_KEEP);
      setDepthFailOp(VK_STENCIL_OP_KEEP);
    }

    // Check if the stencil test for this face is a no-op
    return writeMask() || compareOp() != VK_COMPARE_OP_ALWAYS;
  }


  void DxvkDepthStencilState::normalize() {
    if (depthTest()) {
      // If depth func is equal or if the depth test always fails, depth
      // writes will not have any observable effect so we can skip them.
      if (depthCompareOp() == VK_COMPARE_OP_EQUAL
       || depthCompareOp() == VK_COMPARE_OP_NEVER)
        setDepthWrite(false);

      // If the depth test always passes and no writes are performed, the
      // depth test as a whole is a no-op and can safely be disabled.
      if (depthCompareOp() == VK_COMPARE_OP_ALWAYS && !depthWrite())
        setDepthTest(false);
    } else {
      setDepthWrite(false);
      setDepthCompareOp(VK_COMPARE_OP_ALWAYS);
    }

    if (stencilTest()) {
      // Normalize stencil op and disable stencil testing if both are no-ops.
      bool frontIsNoOp = !m_stencilOpFront.normalize(depthCompareOp());
      bool backIsNoOp = !m_stencilOpBack.normalize(depthCompareOp());

      if (frontIsNoOp && backIsNoOp)
        setStencilTest(false);
    }

    // Normalize stencil ops if stencil test is disabled
    if (!stencilTest()) {
      setStencilOpFront(DxvkStencilOp());
      setStencilOpBack(DxvkStencilOp());
    }
  }


  void DxvkBlendMode::normalize() {
    constexpr VkColorComponentFlags colorMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
    constexpr VkColorComponentFlags alphaMask = VK_COLOR_COMPONENT_A_BIT;

    VkColorComponentFlags newWriteMask = writeMask();

    if (!newWriteMask)
      setBlendEnable(false);

    if (blendEnable()) {
      DxvkBlendNormalizeFlags flags = getBlendNormalizeFlags(
        colorSrcFactor(), colorDstFactor(), colorBlendOp(),
        alphaSrcFactor(), alphaDstFactor(), alphaBlendOp());

      // If alpha or color are effectively not modified given the blend
      // function, set the corresponding part of the write mask to 0.
      if (flags.colorNoOp)
        newWriteMask &= ~colorMask;

      if (flags.alphaNoOp)
        newWriteMask &= ~alphaMask;

      // Check whether blending is equivalent to passing through
      // the source data as if blending was disabled.
      bool needsBlending = false;

      if (newWriteMask & colorMask)
        needsBlending |= !flags.colorPassThrough;

      if (newWriteMask & alphaMask)
        needsBlending |= !flags.alphaPassThrough;

      if (!needsBlending)
        setBlendEnable(false);
    }

    if (!blendEnable() || !(newWriteMask & colorMask))
      setColorOp(VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

    if (!blendEnable() || !(newWriteMask & alphaMask))
      setAlphaOp(VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

    setWriteMask(newWriteMask);
  }

}
