/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsSDL.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"

#include <SDL.h>

using namespace KODI::WINDOWING::SDL;

namespace
{
constexpr uint32_t REPEAT_DELAY_MS = 400;
constexpr uint32_t REPEAT_RATE_MS = 90;
constexpr int16_t AXIS_PRESS = 16000;
constexpr int16_t AXIS_RELEASE = 8000;

struct KeyMap
{
  XBMCKey sym;
  uint16_t unicode;
  bool repeats;
};

// Game controller button -> key in Kodi's default keyboard keymap
KeyMap MapControllerButton(uint8_t button)
{
  switch (button)
  {
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
      return {XBMCK_UP, 0, true};
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
      return {XBMCK_DOWN, 0, true};
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      return {XBMCK_LEFT, 0, true};
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      return {XBMCK_RIGHT, 0, true};
    case SDL_CONTROLLER_BUTTON_A:
      return {XBMCK_RETURN, '\r', false}; // Select
    case SDL_CONTROLLER_BUTTON_B:
      return {XBMCK_BACKSPACE, '\b', false}; // Back
    case SDL_CONTROLLER_BUTTON_X:
      return {XBMCK_c, 'c', false}; // Context menu
    case SDL_CONTROLLER_BUTTON_Y:
      return {XBMCK_i, 'i', false}; // Info
    case SDL_CONTROLLER_BUTTON_START:
      return {XBMCK_SPACE, ' ', false}; // Play / pause
    case SDL_CONTROLLER_BUTTON_BACK:
      return {XBMCK_TAB, '\t', false}; // Toggle fullscreen video / GUI
    case SDL_CONTROLLER_BUTTON_GUIDE:
      return {XBMCK_ESCAPE, 0x1b, false}; // Previous menu / home
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
      return {XBMCK_PAGEUP, 0, true};
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
      return {XBMCK_PAGEDOWN, 0, true};
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
      return {XBMCK_x, 'x', false}; // Stop
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
      return {XBMCK_m, 'm', false}; // Player OSD
    default:
      return {XBMCK_UNKNOWN, 0, false};
  }
}

XBMCKey MapSDLKey(SDL_Keycode key)
{
  // SDL2 keycodes for the printable ASCII range match Kodi's keysyms.
  if (key >= 0x08 && key <= 0x7F)
    return static_cast<XBMCKey>(key);

  switch (key)
  {
    case SDLK_UP: return XBMCK_UP;
    case SDLK_DOWN: return XBMCK_DOWN;
    case SDLK_LEFT: return XBMCK_LEFT;
    case SDLK_RIGHT: return XBMCK_RIGHT;
    case SDLK_PAGEUP: return XBMCK_PAGEUP;
    case SDLK_PAGEDOWN: return XBMCK_PAGEDOWN;
    case SDLK_HOME: return XBMCK_HOME;
    case SDLK_END: return XBMCK_END;
    case SDLK_INSERT: return XBMCK_INSERT;
    case SDLK_MENU: return XBMCK_MENU;
    case SDLK_LSHIFT: return XBMCK_LSHIFT;
    case SDLK_RSHIFT: return XBMCK_RSHIFT;
    case SDLK_LCTRL: return XBMCK_LCTRL;
    case SDLK_RCTRL: return XBMCK_RCTRL;
    case SDLK_LALT: return XBMCK_LALT;
    case SDLK_RALT: return XBMCK_RALT;
    case SDLK_KP_ENTER: return XBMCK_KP_ENTER;
    case SDLK_VOLUMEUP: return XBMCK_VOLUME_UP;
    case SDLK_VOLUMEDOWN: return XBMCK_VOLUME_DOWN;
    case SDLK_MUTE: return XBMCK_VOLUME_MUTE;
    case SDLK_AUDIOPLAY: return XBMCK_MEDIA_PLAY_PAUSE;
    default: break;
  }
  if (key >= SDLK_F1 && key <= SDLK_F12)
    return static_cast<XBMCKey>(XBMCK_F1 + (key - SDLK_F1));
  return XBMCK_UNKNOWN;
}

XBMCMod MapSDLMod(uint16_t mod)
{
  unsigned int m = XBMCKMOD_NONE;
  if (mod & KMOD_LSHIFT) m |= XBMCKMOD_LSHIFT;
  if (mod & KMOD_RSHIFT) m |= XBMCKMOD_RSHIFT;
  if (mod & KMOD_LCTRL) m |= XBMCKMOD_LCTRL;
  if (mod & KMOD_RCTRL) m |= XBMCKMOD_RCTRL;
  if (mod & KMOD_LALT) m |= XBMCKMOD_LALT;
  if (mod & KMOD_RALT) m |= XBMCKMOD_RALT;
  return static_cast<XBMCMod>(m);
}
} // namespace

CWinEventsSDL::CWinEventsSDL()
{
  if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
    CLog::Log(LOGWARNING, "SDL: game controller init failed: {}", SDL_GetError());
  else
    OpenControllers();
}

CWinEventsSDL::~CWinEventsSDL()
{
  for (auto& it : m_controllers)
    SDL_GameControllerClose(it.second);
  m_controllers.clear();
  SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void CWinEventsSDL::OpenControllers()
{
  for (int i = 0; i < SDL_NumJoysticks(); ++i)
    OpenController(i);
}

void CWinEventsSDL::OpenController(int joystickIndex)
{
  if (!SDL_IsGameController(joystickIndex))
  {
    CLog::Log(LOGINFO, "SDL: joystick {} '{}' has no game controller mapping", joystickIndex,
              SDL_JoystickNameForIndex(joystickIndex));
    return;
  }
  SDL_GameController* pad = SDL_GameControllerOpen(joystickIndex);
  if (!pad)
    return;
  const int32_t id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
  if (m_controllers.count(id))
  {
    SDL_GameControllerClose(pad);
    return;
  }
  m_controllers[id] = pad;
  CLog::Log(LOGINFO, "SDL: opened game controller '{}'", SDL_GameControllerName(pad));
}

bool CWinEventsSDL::SendKey(XBMCKey sym, uint16_t unicode, bool down, XBMCMod mod)
{
  if (sym == XBMCK_UNKNOWN)
    return false;

  XBMC_Event ev = {};
  ev.type = down ? XBMC_KEYDOWN : XBMC_KEYUP;
  ev.key.keysym.sym = sym;
  ev.key.keysym.unicode = unicode;
  ev.key.keysym.mod = mod;

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  return appPort ? appPort->OnEvent(ev) : false;
}

bool CWinEventsSDL::OnControllerButton(uint8_t button, bool down)
{
  const KeyMap k = MapControllerButton(button);
  if (k.sym == XBMCK_UNKNOWN)
    return false;

  if (down && k.repeats)
  {
    m_heldKey = k.sym;
    m_heldUnicode = k.unicode;
    m_nextRepeat = SDL_GetTicks() + REPEAT_DELAY_MS;
  }
  else if (!down && m_heldKey == k.sym)
  {
    m_heldKey = XBMCK_UNKNOWN;
  }
  return SendKey(k.sym, k.unicode, down);
}

bool CWinEventsSDL::OnControllerAxis(uint8_t axis, int16_t value)
{
  // Left stick doubles as a d-pad.
  if (axis != SDL_CONTROLLER_AXIS_LEFTX && axis != SDL_CONTROLLER_AXIS_LEFTY)
    return false;

  int& state = m_axisState[axis == SDL_CONTROLLER_AXIS_LEFTX ? 0 : 1];
  int dir = state;
  if (value > AXIS_PRESS)
    dir = 1;
  else if (value < -AXIS_PRESS)
    dir = -1;
  else if (value > -AXIS_RELEASE && value < AXIS_RELEASE)
    dir = 0;

  if (dir == state)
    return false;

  const bool horizontal = axis == SDL_CONTROLLER_AXIS_LEFTX;
  auto keyFor = [horizontal](int d) {
    if (horizontal)
      return d > 0 ? SDL_CONTROLLER_BUTTON_DPAD_RIGHT : SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    return d > 0 ? SDL_CONTROLLER_BUTTON_DPAD_DOWN : SDL_CONTROLLER_BUTTON_DPAD_UP;
  };

  bool ret = false;
  if (state != 0)
    ret |= OnControllerButton(keyFor(state), false);
  state = dir;
  if (state != 0)
    ret |= OnControllerButton(keyFor(state), true);
  return ret;
}

bool CWinEventsSDL::HandleRepeat()
{
  if (m_heldKey == XBMCK_UNKNOWN)
    return false;
  const uint32_t now = SDL_GetTicks();
  if (static_cast<int32_t>(now - m_nextRepeat) < 0)
    return false;
  m_nextRepeat = now + REPEAT_RATE_MS;
  return SendKey(m_heldKey, m_heldUnicode, true);
}

bool CWinEventsSDL::MessagePump()
{
  bool ret = false;
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_QUIT:
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
        ret = true;
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP:
      {
        const XBMCKey sym = MapSDLKey(event.key.keysym.sym);
        uint16_t unicode = 0;
        if (sym < 0x80)
          unicode = static_cast<uint16_t>(sym);
        ret |= SendKey(sym, unicode, event.type == SDL_KEYDOWN, MapSDLMod(event.key.keysym.mod));
        break;
      }

      case SDL_CONTROLLERDEVICEADDED:
        OpenController(event.cdevice.which);
        break;

      case SDL_CONTROLLERDEVICEREMOVED:
      {
        auto it = m_controllers.find(event.cdevice.which);
        if (it != m_controllers.end())
        {
          SDL_GameControllerClose(it->second);
          m_controllers.erase(it);
        }
        break;
      }

      case SDL_CONTROLLERBUTTONDOWN:
      case SDL_CONTROLLERBUTTONUP:
        ret |= OnControllerButton(event.cbutton.button, event.type == SDL_CONTROLLERBUTTONDOWN);
        break;

      case SDL_CONTROLLERAXISMOTION:
        ret |= OnControllerAxis(event.caxis.axis, event.caxis.value);
        break;

      default:
        break;
    }
  }

  ret |= HandleRepeat();
  return ret;
}
