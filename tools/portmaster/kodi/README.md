## Notes

Kodi 21.3 "Omega" built for PortMaster handhelds, with an SDL2 + OpenGL ES display
backend (for firmwares without DRM/KMS, such as muOS on Allwinner H700 devices) and
hardware H.264 / HEVC decoding through [libpmhwdec](https://github.com/Knifethrower/pmhwdec).

Source: https://github.com/Kippykip/xbmc_muOS

Thanks to Team Kodi for Kodi, and to the libpmhwdec author for the hardware decoder.

## Controls

| Button | Action |
|--|--|
| D-pad / left stick | Navigate |
| A | Select |
| B | Back |
| X | Context menu |
| Y | Info |
| Start | Play / pause |
| Select | Toggle fullscreen video / menus |
| L1 / R1 | Page up / page down |
| L3 | Stop |
| R3 | Player OSD |

Set `KODI_PMHWDEC=0` in `Kodi.sh` to force software decoding.
