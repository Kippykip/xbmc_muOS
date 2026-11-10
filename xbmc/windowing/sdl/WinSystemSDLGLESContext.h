/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/gles/RenderSystemGLES.h"
#include "threads/CriticalSection.h"
#include "windowing/WinSystem.h"

#include <memory>
#include <string>
#include <vector>

struct SDL_Window;
typedef void* SDL_GLContext;

class IDispResource;

namespace KODI
{
namespace WINDOWING
{
namespace SDL
{

/*!
 * \brief Window system that lets SDL2 own the display and the GLES context.
 *
 * Meant for devices where the only display path is whatever SDL2 driver the
 * firmware ships (e.g. the "mali" fbdev driver on muOS for Allwinner H700).
 * There is a single fullscreen mode: the panel's native resolution.
 */
class CWinSystemSDLGLESContext : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CWinSystemSDLGLESContext();
  ~CWinSystemSDLGLESContext() override;

  static void Register();
  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  const std::string GetName() override { return "sdl"; }
  CRenderSystemBase* GetRenderSystem() override { return this; }

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void UpdateResolutions() override;

  bool CanDoWindowed() override { return false; }
  bool HasCursor() override { return false; }
  bool SupportsScreenMove() override { return false; }

  bool MessagePump() override;

  void Register(IDispResource* resource) override;
  void Unregister(IDispResource* resource) override;

protected:
  void SetVSyncImpl(bool enable) override;
  void PresentRenderImpl(bool rendered) override;

private:
  bool GetNativeMode(int& width, int& height, float& refresh) const;

  SDL_Window* m_window{nullptr};
  SDL_GLContext m_glContext{nullptr};

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
};

} // namespace SDL
} // namespace WINDOWING
} // namespace KODI
