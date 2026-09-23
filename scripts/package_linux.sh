#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build-release}"
output_dir="${2:-dist}"
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/$output_dir"

DESTDIR="$root/AppDir" cmake --install "$root/$build_dir"
desktop="$root/AppDir/usr/share/applications/nebipadplus.desktop"
icon="$root/AppDir/usr/share/icons/hicolor/512x512/apps/nebipadplus.png"

curl -L -o "$root/linuxdeploy.AppImage" https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
curl -L -o "$root/linuxdeploy-plugin-qt" https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x "$root/linuxdeploy.AppImage" "$root/linuxdeploy-plugin-qt"
export PATH="$root:$PATH"
export LINUXDEPLOY_PLUGIN_QT_QMAKE="$(command -v qmake6 || command -v qmake)"
export OUTPUT="$root/$output_dir/NebiPadPlusSuite-Linux-x86_64.AppImage"
"$root/linuxdeploy.AppImage" --appdir "$root/AppDir" --desktop-file "$desktop" --icon-file "$icon" --plugin qt --output appimage

dpkg_root="$root/$output_dir/deb-root"
mkdir -p "$dpkg_root/DEBIAN"
cp -a "$root/AppDir/usr" "$dpkg_root/"
cat > "$dpkg_root/DEBIAN/control" <<'EOF'
Package: nebipadplus-suite
Version: 0.1.0
Section: editors
Priority: optional
Architecture: amd64
Maintainer: Costa Fabio
Depends: libc6, libstdc++6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6network6, libqt6svg6, libqt6multimedia6, libqt6printsupport6, libqt6charts6
Description: NebiPad+ cross-platform productivity suite
 Text editor, diagram editor, page layout workspace and mathematical editor.
EOF
dpkg-deb --build --root-owner-group "$dpkg_root" "$root/$output_dir/NebiPadPlusSuite-Linux-amd64.deb"
