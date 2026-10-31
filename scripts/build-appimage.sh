#!/usr/bin/env bash
set -euo pipefail

# Build Pan Loop in Release mode and package it as an x86_64 AppImage.
# Requirements: the normal Pan Loop build dependencies plus wget and either
# rsvg-convert or ImageMagick. The AppImage tooling is downloaded on demand.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-release}"
DIST_DIR="${DIST_DIR:-$ROOT/dist}"
TOOLS_DIR="${TOOLS_DIR:-$ROOT/.cache/appimage-tools}"
APPDIR="${APPDIR:-$ROOT/build/appimage/PanLoop.AppDir}"

PROJECT_VERSION="$(sed -n 's/^project(PanLoop VERSION \([0-9][0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
PROJECT_VERSION="${PROJECT_VERSION:-0.1.0}"

if [[ "${PANLOOP_SKIP_BUILD:-0}" != "1" ]]; then
    echo "==> Building Pan Loop $PROJECT_VERSION (Release)"
    cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DPANLOOP_BUILD_TESTS=OFF
    cmake --build "$BUILD_DIR" -j"$(nproc)"
fi

APP_BINARY="$BUILD_DIR/pan-loop_artefacts/Release/pan-loop"

if [[ ! -x "$APP_BINARY" ]]; then
    echo "Pan Loop binary not found at: $APP_BINARY" >&2
    echo "Build it first or set BUILD_DIR to the Release build directory." >&2
    exit 1
fi

for command in wget; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "Required command not found: $command" >&2
        exit 1
    fi
done

mkdir -p "$TOOLS_DIR" "$DIST_DIR"
rm -rf "$APPDIR"

install -d "$APPDIR/usr/bin"
install -d "$APPDIR/usr/share/applications"
install -d "$APPDIR/usr/share/icons/hicolor/256x256/apps"
install -d "$APPDIR/usr/share/icons/hicolor/512x512/apps"
install -d "$APPDIR/usr/share/icons/hicolor/scalable/apps"
install -d "$APPDIR/usr/share/doc/pan-loop"

install -m 755 "$APP_BINARY" "$APPDIR/usr/bin/pan-loop"
install -m 644 "$ROOT/packaging/pan-loop.desktop" "$APPDIR/pan-loop.desktop"
install -m 644 "$ROOT/packaging/pan-loop.desktop" "$APPDIR/usr/share/applications/pan-loop.desktop"
install -m 644 "$ROOT/packaging/pan-loop.svg" "$APPDIR/usr/share/icons/hicolor/scalable/apps/pan-loop.svg"
install -m 644 "$ROOT/LICENSE" "$APPDIR/usr/share/doc/pan-loop/LICENSE"

render_icon()
{
    local size="$1"
    local output="$2"

    if command -v rsvg-convert >/dev/null 2>&1; then
        rsvg-convert -w "$size" -h "$size" "$ROOT/packaging/pan-loop.svg" -o "$output"
    elif command -v magick >/dev/null 2>&1; then
        magick -background none "$ROOT/packaging/pan-loop.svg" -resize "${size}x${size}" "$output"
    elif command -v convert >/dev/null 2>&1; then
        convert -background none "$ROOT/packaging/pan-loop.svg" -resize "${size}x${size}" "$output"
    else
        echo "Install rsvg-convert or ImageMagick to render the application icon." >&2
        exit 1
    fi
}

render_icon 256 "$APPDIR/usr/share/icons/hicolor/256x256/apps/pan-loop.png"
render_icon 512 "$APPDIR/usr/share/icons/hicolor/512x512/apps/pan-loop.png"
render_icon 256 "$APPDIR/pan-loop.png"
cp "$APPDIR/pan-loop.png" "$APPDIR/.DirIcon"

download_appimage_tool()
{
    local name="$1"
    local url="$2"
    local destination="$TOOLS_DIR/$name"

    if [[ -x "$destination" ]]; then
        echo "$destination"
        return
    fi

    echo "==> Downloading $name" >&2
    wget -q --show-progress -O "$destination.part" "$url"
    chmod +x "$destination.part"
    mv "$destination.part" "$destination"
    echo "$destination"
}

LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
APPIMAGETOOL_URL="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"

LINUXDEPLOY="$(download_appimage_tool linuxdeploy-x86_64.AppImage "$LINUXDEPLOY_URL")"
APPIMAGETOOL="$(download_appimage_tool appimagetool-x86_64.AppImage "$APPIMAGETOOL_URL")"

echo "==> Collecting runtime libraries with linuxdeploy"
(
    cd "$APPDIR"
    APPIMAGE_EXTRACT_AND_RUN=1 "$LINUXDEPLOY" \
        --appdir "$APPDIR" \
        --executable "$APPDIR/usr/bin/pan-loop" \
        --desktop-file "$APPDIR/pan-loop.desktop" \
        --icon-file "$APPDIR/pan-loop.png"
)

OUTPUT="$DIST_DIR/Pan-Loop-${PROJECT_VERSION}-x86_64.AppImage"
rm -f "$OUTPUT"

echo "==> Creating AppImage"
(
    cd "$DIST_DIR"
    ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 "$APPIMAGETOOL" \
        --no-appstream "$APPDIR" "$OUTPUT"
)

echo
echo "AppImage created:"
ls -lh "$OUTPUT"
