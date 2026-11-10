/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemSDLGLESContext.h"

#include "WinEventsSDL.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/Process/sdl/ProcessInfoSDL.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "guilib/DispResource.h"
#include "rendering/gles/ScreenshotSurfaceGLES.h"
#include "settings/DisplaySettings.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WindowSystemFactory.h"

#include <algorithm>
#include <mutex>

#include <SDL.h>

using namespace KODI::WINDOWING::SDL;

namespace
{
constexpr float DEFAULT_REFRESH = 60.0f;
}

void CWinSystemSDLGLESContext::Register()
{
  CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem, "sdl");
}

std::unique_ptr<CWinSystemBase> CWinSystemSDLGLESContext::CreateWinSystem()
{
  return std::make_unique<CWinSystemSDLGLESContext>();
}

CWinSystemSDLGLESContext::CWinSystemSDLGLESContext()
{
  m_winEvents = std::make_unique<CWinEventsSDL>();
}

CWinSystemSDLGLESContext::~CWinSystemSDLGLESContext()
{
  DestroyWindow();
}

bool CWinSystemSDLGLESContext::InitWindowSystem()
{
  // Keep the console/framebuffer from blanking while Kodi is up.
  SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");

  if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
  {
    CLog::Log(LOGERROR, "SDL: SDL_InitSubSystem(VIDEO) failed: {}", SDL_GetError());
    return false;
  }
  CLog::Log(LOGINFO, "SDL: video driver '{}'", SDL_GetCurrentVideoDriver());

  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CDVDFactoryCodec::ClearHWAccels();
  CLinuxRendererGLES::Register();
  VIDEOPLAYER::CProcessInfoSDL::Register();
  CScreenshotSurfaceGLES::Register();

  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemSDLGLESContext::DestroyWindowSystem()
{
  DestroyWindow();
  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  return CWinSystemBase::DestroyWindowSystem();
}

bool CWinSystemSDLGLESContext::GetNativeMode(int& width, int& height, float& refresh) const
{
  SDL_DisplayMode mode;
  if (SDL_GetDesktopDisplayMode(0, &mode) != 0)
  {
    CLog::Log(LOGERROR, "SDL: SDL_GetDesktopDisplayMode failed: {}", SDL_GetError());
    return false;
  }
  width = mode.w;
  height = mode.h;
  refresh = mode.refresh_rate > 0 ? static_cast<float>(mode.refresh_rate) : DEFAULT_REFRESH;
  return width > 0 && height > 0;
}

void CWinSystemSDLGLESContext::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  int w = 0, h = 0;
  float refresh = DEFAULT_REFRESH;
  if (!GetNativeMode(w, h, refresh))
    return;

  RESOLUTION_INFO& desktop = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
  UpdateDesktopResolution(desktop, "SDL", w, h, refresh, 0);
  desktop.bFullScreen = true;
  CLog::Log(LOGINFO, "SDL: native display mode {}x{} @ {:.2f} Hz", w, h, refresh);

  CDisplaySettings::GetInstance().ClearCustomResolutions();
}

bool CWinSystemSDLGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  int w = 0, h = 0;
  float refresh = DEFAULT_REFRESH;
  if (!GetNativeMode(w, h, refresh))
  {
    w = res.iWidth;
    h = res.iHeight;
  }

  if (!m_window)
  {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_window = SDL_CreateWindow(name.empty() ? "Kodi" : name.c_str(), SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED, w, h,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
    if (!m_window)
    {
      CLog::Log(LOGERROR, "SDL: SDL_CreateWindow failed: {}", SDL_GetError());
      return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext)
    {
      CLog::Log(LOGERROR, "SDL: SDL_GL_CreateContext failed: {}", SDL_GetError());
      SDL_DestroyWindow(m_window);
      m_window = nullptr;
      return false;
    }
    SDL_ShowCursor(SDL_DISABLE);
  }

  SDL_GL_MakeCurrent(m_window, m_glContext);
  SDL_GL_GetDrawableSize(m_window, &w, &h);

  UpdateDesktopResolution(res, "SDL", w, h, refresh, 0);
  res.bFullScreen = true;

  m_nWidth = w;
  m_nHeight = h;
  m_bFullScreen = true;
  m_bWindowCreated = true;

  CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = res;
  SetWindowResolution(w, h);

  CLog::Log(LOGINFO, "SDL: created GLES window {}x{}", w, h);
  return true;
}

bool CWinSystemSDLGLESContext::DestroyWindow()
{
  if (m_glContext)
  {
    SDL_GL_MakeCurrent(m_window, nullptr);
    SDL_GL_DeleteContext(m_glContext);
    m_glContext = nullptr;
  }
  if (m_window)
  {
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }
  m_bWindowCreated = false;
  return true;
}

bool CWinSystemSDLGLESContext::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  // Fixed-size fullscreen display: nothing to resize, just keep the render
  // system in sync with the real surface.
  CRenderSystemGLES::ResetRenderSystem(m_nWidth, m_nHeight);
  return true;
}

bool CWinSystemSDLGLESContext::SetFullScreen(bool fullScreen,
                                             RESOLUTION_INFO& res,
                                             bool blankOtherDisplays)
{
  if (!m_window && !CreateNewWindow("", true, res))
    return false;

  {
    std::unique_lock<CCriticalSection> lock(m_resourceSection);
    for (auto resource : m_resources)
      resource->OnLostDisplay();
  }

  CRenderSystemGLES::ResetRenderSystem(m_nWidth, m_nHeight);

  {
    std::unique_lock<CCriticalSection> lock(m_resourceSection);
    for (auto resource : m_resources)
      resource->OnResetDisplay();
  }
  return true;
}

bool CWinSystemSDLGLESContext::MessagePump()
{
  return m_winEvents ? m_winEvents->MessagePump() : false;
}

void CWinSystemSDLGLESContext::Register(IDispResource* resource)
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemSDLGLESContext::Unregister(IDispResource* resource)
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  m_resources.erase(std::remove(m_resources.begin(), m_resources.end(), resource),
                    m_resources.end());
}

void CWinSystemSDLGLESContext::SetVSyncImpl(bool enable)
{
  if (SDL_GL_SetSwapInterval(enable ? 1 : 0) != 0)
    CLog::Log(LOGDEBUG, "SDL: SDL_GL_SetSwapInterval({}) failed: {}", enable, SDL_GetError());
}

void CWinSystemSDLGLESContext::PresentRenderImpl(bool rendered)
{
  if (rendered && m_window)
    SDL_GL_SwapWindow(m_window);
}
