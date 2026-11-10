#!/bin/bash
# Kodi for PortMaster (muOS and other CFWs whose SDL2 has a working GLES driver)

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR="/$directory/ports/kodi"
CONFDIR="$GAMEDIR/conf"
mkdir -p "$CONFDIR"
cd "$GAMEDIR"

> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# Kodi keeps its userdata (settings, add-ons, library) in $HOME/.kodi
export HOME="$CONFDIR"

export KODI_HOME="$GAMEDIR/kodi/share/kodi"
export KODI_BIN_HOME="$GAMEDIR/kodi/lib/kodi"
export PYTHONHOME="$GAMEDIR/kodi"
export LD_LIBRARY_PATH="$GAMEDIR/libs.${DEVICE_ARCH}:$GAMEDIR/kodi/lib:$LD_LIBRARY_PATH"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"

# Hardware video decoding (libpmhwdec). Set to 0 to force software decoding.
export KODI_PMHWDEC=${KODI_PMHWDEC:-1}

# Kodi reads the pad through SDL itself; gptokeyb is only here for the exit hotkey.
$GPTOKEYB "kodi.${DEVICE_ARCH}" &
pm_platform_helper "$GAMEDIR/kodi.${DEVICE_ARCH}"

./kodi.${DEVICE_ARCH} --standalone --audio-backend=alsa

pm_finish
