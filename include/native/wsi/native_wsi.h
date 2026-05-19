#pragma once

#ifdef DXVK_WSI_WIN32
#include <windows.h>
namespace dxvk::wsi {
  inline HWND fromHwnd(HWND hWindow) {
    return hWindow;
  }
  inline HWND toHwnd(HWND hWindow) {
    return hWindow;
  }
  inline HMONITOR fromHmonitor(HMONITOR hMonitor) {
    return hMonitor;
  }
  inline HMONITOR toHmonitor(HMONITOR hMonitor) {
    return hMonitor;
  }
}
#elif DXVK_WSI_SDL3
#include "wsi/native_sdl3.h"
#elif DXVK_WSI_SDL2
#include "wsi/native_sdl2.h"
#elif DXVK_WSI_GLFW
#include "wsi/native_glfw.h"
#else
#error Unknown wsi!
#endif