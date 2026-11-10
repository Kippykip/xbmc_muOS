/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/XBMC_keysym.h"
#include "windowing/WinEvents.h"

#include <cstdint>
#include <map>

typedef struct _SDL_GameController SDL_GameController;

namespace KODI
{
namespace WINDOWING
{
namespace SDL
{

/*!
 * \brief Turns SDL keyboard and game controller events into Kodi key events.
 *
 * Game controller buttons are translated to the keys of Kodi's default
 * keyboard keymap, so a handheld works out of the box without gptokeyb
 * or the peripheral.joystick add-on.
 */
class CWinEventsSDL : public IWinEvents
{
public:
  CWinEventsSDL();
  ~CWinEventsSDL() override;

  bool MessagePump() override;

private:
  bool SendKey(XBMCKey sym, uint16_t unicode, bool down, XBMCMod mod = XBMCKMOD_NONE);
  bool OnControllerButton(uint8_t button, bool down);
  bool OnControllerAxis(uint8_t axis, int16_t value);
  bool HandleRepeat();
  void OpenControllers();
  void OpenController(int joystickIndex);

  std::map<int32_t, SDL_GameController*> m_controllers; // keyed by SDL instance id

  // Key auto-repeat for held controller buttons / sticks
  XBMCKey m_heldKey{XBMCK_UNKNOWN};
  uint16_t m_heldUnicode{0};
  uint32_t m_nextRepeat{0};

  // Last direction reported per stick axis: -1, 0 or +1
  int m_axisState[2]{0, 0};
};

} // namespace SDL
} // namespace WINDOWING
} // namespace KODI
