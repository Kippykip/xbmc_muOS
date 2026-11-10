#!/bin/bash
# Assemble the PortMaster "kodi" port from a cross-compiled Kodi install.
#
#   package.sh <kodi install prefix> <sysroot> <extra lib dir>... -o <output dir>
#
# - <kodi install prefix>: the CMAKE_INSTALL_PREFIX Kodi was installed to (DESTDIR'd)
# - <sysroot>: root of the aarch64 build environment, used to find shared libraries
# - extra lib dirs: searched first (e.g. the FFmpeg / libpmhwdec build output)
#
# Every shared library Kodi needs is copied into libs.aarch64/, except the ones
# every PortMaster CFW provides itself (glibc, libstdc++, GLES/EGL, SDL2, ALSA).
set -euo pipefail

PREFIX=$1; SYSROOT=$2; shift 2
EXTRA=()
OUT=""
while [ $# -gt 0 ]; do
  case "$1" in
    -o) OUT=$2; shift 2 ;;
    *) EXTRA+=("$1"); shift ;;
  esac
done
[ -n "$OUT" ] || { echo "missing -o <output dir>"; exit 1; }

HERE=$(cd "$(dirname "$0")" && pwd)
PORT="$OUT/kodi"
READELF=${READELF:-aarch64-linux-gnu-readelf}
STRIP=${STRIP:-aarch64-linux-gnu-strip}

rm -rf "$PORT"
mkdir -p "$PORT/kodi" "$PORT/libs.aarch64" "$PORT/licenses" "$PORT/conf"

# Kodi itself: binary, share/kodi (skins, system files), lib/kodi (helpers, add-ons)
mkdir -p "$PORT/kodi/share" "$PORT/kodi/lib"
cp -a "$PREFIX/share/kodi" "$PORT/kodi/share/"
cp -a "$PREFIX/lib/kodi" "$PORT/kodi/lib/"
mv "$PORT/kodi/lib/kodi/kodi-sdl" "$PORT/kodi.aarch64"

# Libraries the CFW provides: never bundle these.
SKIP='^(ld-linux-aarch64|libc|libm|libdl|libpthread|librt|libresolv|libutil|libstdc\+\+|libgcc_s|libGLESv2|libGLESv1_CM|libEGL|libmali|libSDL2-2\.0|libasound|libdrm)\.so'

SEARCH=("${EXTRA[@]}" "$SYSROOT/usr/lib/aarch64-linux-gnu" "$SYSROOT/lib/aarch64-linux-gnu"
        "$SYSROOT/usr/lib/aarch64-linux-gnu/pulseaudio" "$SYSROOT/usr/lib")

find_lib() {
  for d in "${SEARCH[@]}"; do
    # multiarch sysroots mix architectures in /usr/lib: only take AArch64 objects
    if [ -e "$d/$1" ] && $READELF -h "$d/$1" 2>/dev/null | grep -q AArch64; then
      echo "$d/$1"; return 0
    fi
  done
  return 1
}

declare -A SEEN
queue=("$PORT/kodi.aarch64")
while [ ${#queue[@]} -gt 0 ]; do
  f=${queue[0]}; queue=("${queue[@]:1}")
  for need in $($READELF -d "$f" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p'); do
    [[ -n "${SEEN[$need]:-}" ]] && continue
    SEEN[$need]=1
    [[ "$need" =~ $SKIP ]] && continue
    src=$(find_lib "$need") || { echo "WARNING: $need not found"; continue; }
    cp -L "$src" "$PORT/libs.aarch64/$need"
    queue+=("$PORT/libs.aarch64/$need")
  done
done

# libpmhwdec is dlopen()ed by FFmpeg, and starts pmhwdec-cedard from its own directory.
for d in "${EXTRA[@]}"; do
  [ -e "$d/libpmhwdec.so.1" ] && cp -L "$d/libpmhwdec.so.1" "$PORT/libs.aarch64/"
  [ -e "$d/pmhwdec-cedard" ] && cp -L "$d/pmhwdec-cedard" "$PORT/libs.aarch64/"
done

# Python standard library for add-ons (Kodi embeds the interpreter).
PYVER=$(ls "$PORT/libs.aarch64" | sed -n 's/^libpython\(3\.[0-9]*\).*/\1/p' | head -1)
if [ -n "$PYVER" ]; then
  mkdir -p "$PORT/kodi/lib/python$PYVER"
  cp -a "$SYSROOT/usr/lib/python$PYVER/." "$PORT/kodi/lib/python$PYVER/"
  rm -rf "$PORT/kodi/lib/python$PYVER"/{test,idlelib,tkinter,turtledemo,ensurepip,lib2to3,config-*}
  find "$PORT/kodi/lib/python$PYVER" -name "__pycache__" -type d -prune -exec rm -rf {} +
  # binary modules (_ssl, _sqlite3, ...) live in lib-dynload
  if [ -d "$SYSROOT/usr/lib/python$PYVER/lib-dynload" ]; then
    cp -a "$SYSROOT/usr/lib/python$PYVER/lib-dynload" "$PORT/kodi/lib/python$PYVER/"
  fi
  # host-architecture extension modules from a multiarch sysroot
  find "$PORT/kodi/lib/python$PYVER" -name "*-x86_64-linux-gnu.so" -delete
fi

$STRIP --strip-unneeded "$PORT/kodi.aarch64" "$PORT"/libs.aarch64/*.so* 2>/dev/null || true

# Port metadata and launcher
cp "$HERE/kodi/Kodi.sh" "$OUT/Kodi.sh"
chmod +x "$OUT/Kodi.sh" "$PORT/kodi.aarch64"
for f in port.json README.md gameinfo.xml; do
  [ -e "$HERE/kodi/$f" ] && cp "$HERE/kodi/$f" "$PORT/"
done
cp "$HERE/../../LICENSE.md" "$PORT/licenses/Kodi-LICENSE.md"
cp -a "$HERE/../../LICENSES" "$PORT/licenses/Kodi-LICENSES"

du -sh "$PORT"
echo "Port assembled in $OUT"
