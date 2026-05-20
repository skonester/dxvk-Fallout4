#include "d3d11_state.h"

#include "../dxvk/dxvk_hash.h"
#include "../util/util_bit.h"
#include <cstring>

namespace dxvk {
  
  size_t D3D11StateDescHash::operator () (
    const D3D11_BLEND_DESC1& desc) const {
    DxvkHashState hash;
    hash.add(desc.AlphaToCoverageEnable);
    hash.add(desc.IndependentBlendEnable);
    
    // Render targets 1 to 7 are ignored and may contain
    // undefined data if independent blend is disabled
    const uint32_t usedRenderTargets = desc.IndependentBlendEnable ? 8 : 1;
    
    for (uint32_t i = 0; i < usedRenderTargets; i++)
      hash.add(this->operator () (desc.RenderTarget[i]));
    
    return hash;
  }
  
  
  size_t D3D11StateDescHash::operator () (
    const D3D11_DEPTH_STENCILOP_DESC& desc) const {
    return bit::crc32_hash(reinterpret_cast<const char*>(&desc), sizeof(desc));
  }
  
  
  size_t D3D11StateDescHash::operator () (
    const D3D11_DEPTH_STENCIL_DESC& desc) const {
    DxvkHashState hash;
    hash.add(bit::crc32_hash(reinterpret_cast<const char*>(&desc), 16));
    hash.add((uint32_t(desc.StencilReadMask) << 8) | desc.StencilWriteMask);
    hash.add(bit::crc32_hash(reinterpret_cast<const char*>(&desc.FrontFace), sizeof(desc.FrontFace) * 2));
    return hash;
  }
  
  
  size_t D3D11StateDescHash::operator () (
    const D3D11_RASTERIZER_DESC2& desc) const {
    return bit::crc32_hash(reinterpret_cast<const char*>(&desc), sizeof(desc));
  }
  
  
  size_t D3D11StateDescHash::operator () (
    const D3D11_RENDER_TARGET_BLEND_DESC1& desc) const {
    DxvkHashState hash;
    hash.add(bit::crc32_hash(reinterpret_cast<const char*>(&desc), 36));
    hash.add(desc.RenderTargetWriteMask);
    return hash;
  }
  
  
  size_t D3D11StateDescHash::operator () (
    const D3D11_SAMPLER_DESC& desc) const {
    return bit::crc32_hash(reinterpret_cast<const char*>(&desc), sizeof(desc));
  }
  
  
  bool D3D11StateDescEqual::operator () (
    const D3D11_BLEND_DESC1& a,
    const D3D11_BLEND_DESC1& b) const {
    if (a.AlphaToCoverageEnable  != b.AlphaToCoverageEnable
     || a.IndependentBlendEnable != b.IndependentBlendEnable)
      return false;
    
    // Render targets 1 to 7 are ignored and may contain
    // undefined data if independent blend is disabled
    const uint32_t usedRenderTargets = a.IndependentBlendEnable ? 8 : 1;
    
    for (uint32_t i = 0; i < usedRenderTargets; i++) {
      if (!this->operator () (a.RenderTarget[i], b.RenderTarget[i]))
        return false;
    }
    
    return true;
  }
  
  
  bool D3D11StateDescEqual::operator () (
    const D3D11_DEPTH_STENCILOP_DESC& a,
    const D3D11_DEPTH_STENCILOP_DESC& b) const {
    return std::memcmp(&a, &b, sizeof(a)) == 0;
  }
  
  
  bool D3D11StateDescEqual::operator () (
    const D3D11_DEPTH_STENCIL_DESC& a,
    const D3D11_DEPTH_STENCIL_DESC& b) const {
    return std::memcmp(&a, &b, 16) == 0
        && a.StencilReadMask  == b.StencilReadMask
        && a.StencilWriteMask == b.StencilWriteMask
        && std::memcmp(&a.FrontFace, &b.FrontFace, sizeof(a.FrontFace) * 2) == 0;
  }
  
  
  bool D3D11StateDescEqual::operator () (
    const D3D11_RASTERIZER_DESC2& a,
    const D3D11_RASTERIZER_DESC2& b) const {
    return std::memcmp(&a, &b, sizeof(a)) == 0;
  }
  
  
  bool D3D11StateDescEqual::operator () (
    const D3D11_RENDER_TARGET_BLEND_DESC1& a,
    const D3D11_RENDER_TARGET_BLEND_DESC1& b) const {
    return std::memcmp(&a, &b, 36) == 0
        && a.RenderTargetWriteMask == b.RenderTargetWriteMask;
  }
  
  
  bool D3D11StateDescEqual::operator () (
    const D3D11_SAMPLER_DESC& a,
    const D3D11_SAMPLER_DESC& b) const {
    return std::memcmp(&a, &b, sizeof(a)) == 0;
  }
  
}
