#!/bin/sh
set -eu

app_path="${1:-build-release/NebiPadPlus.app}"
qt_deploy="${QT_MACDEPLOYQT:-$(command -v macdeployqt)}"
qt_paths="${QT_QTPATHS:-$(command -v qtpaths6 || command -v qtpaths)}"
qt_plugins="${QT_PLUGIN_PATH:-$($qt_paths --plugin-dir)}"
qt_libs="${QT_LIB_PATH:-$($qt_paths --query QT_INSTALL_LIBS)}"

deploy_libpaths="-libpath=$qt_libs"
for dependency in webp brotli; do
    if command -v brew >/dev/null 2>&1; then
        dependency_prefix="$(brew --prefix "$dependency" 2>/dev/null || true)"
        if [ -d "$dependency_prefix/lib" ]; then
            deploy_libpaths="$deploy_libpaths -libpath=$dependency_prefix/lib"
        fi
    fi
done

# Copy the native macOS TLS backend before the single macdeployqt pass: without
# it QtNetwork is present but every HTTPS request fails at initialization.
tls_executable=""
if [ -f "$qt_plugins/tls/libqsecuretransportbackend.dylib" ]; then
    mkdir -p "$app_path/Contents/PlugIns/tls"
    cp "$qt_plugins/tls/libqsecuretransportbackend.dylib" "$app_path/Contents/PlugIns/tls/"
    tls_executable="-executable=$app_path/Contents/PlugIns/tls/libqsecuretransportbackend.dylib"
fi

# Word splitting is intentional: macdeployqt accepts repeated -libpath options.
# shellcheck disable=SC2086
"$qt_deploy" "$app_path" -always-overwrite $tls_executable $deploy_libpaths

# Homebrew's WebP bottle keeps sharp-yuv as @rpath, while third party dylibs
# live together in Contents/Frameworks inside the application bundle.
webp="$app_path/Contents/Frameworks/libwebp.7.dylib"
if [ -f "$webp" ] && [ -f "$app_path/Contents/Frameworks/libsharpyuv.0.dylib" ]; then
    install_name_tool -change @rpath/libsharpyuv.0.dylib \
        @loader_path/libsharpyuv.0.dylib "$webp"
fi

# Homebrew libraries use @rpath among peer libraries. Inside the bundle they
# are siblings, so resolve those references relative to the loading library.
for binary in "$app_path"/Contents/Frameworks/*.dylib; do
    [ -f "$binary" ] || continue
    for dependency in libbrotlicommon.1.dylib libsharpyuv.0.dylib libwebp.7.dylib; do
        if otool -L "$binary" | grep -q "@rpath/$dependency" && \
           [ -f "$app_path/Contents/Frameworks/$dependency" ]; then
            install_name_tool -change "@rpath/$dependency" \
                "@loader_path/$dependency" "$binary"
        fi
    done
done

# Sign nested Qt frameworks first, then give Chromium's helper its required
# entitlements and reseal the containing framework and application.
codesign --force --deep --sign - "$app_path"
core="$app_path/Contents/Frameworks/QtWebEngineCore.framework"
helper="$core/Versions/A/Helpers/QtWebEngineProcess.app"
if [ -d "$helper" ]; then
    codesign --force --sign - \
        --entitlements "$helper/Contents/Resources/QtWebEngineProcess.entitlements" \
        "$helper"
    codesign --force --sign - "$core"
    codesign --force --sign - "$app_path"
fi
codesign --verify --deep --strict "$app_path"
