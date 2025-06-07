#!/bin/sh
set -e

# Create an AppImage from a built fhDOOM binary.
# The script expects the binary to reside at release/fhDOOM
# and game data to be present in release/base.
# Set APPIMAGE_TOOL to override the path to the appimagetool utility.

APPIMAGE_TOOL=${APPIMAGE_TOOL:-appimagetool}

if [ ! -x "release/fhDOOM" ]; then
  echo "release/fhDOOM not found" >&2
  exit 1
fi

WORKDIR=$(mktemp -d)
APPDIR="$WORKDIR/fhDOOM.AppDir"

mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/fhdoom"

cp release/fhDOOM "$APPDIR/usr/bin/"
cp -r release/base "$APPDIR/usr/share/fhdoom/"

cat > "$APPDIR/fhDOOM.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=fhDOOM
Exec=fhDOOM
Icon=fhdoom
Categories=Game;
DESKTOP

cp resources/icon.png "$APPDIR/fhdoom.png"

"$APPIMAGE_TOOL" "$APPDIR" fhDOOM.AppImage

echo "AppImage written to fhDOOM.AppImage"

