#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CUBE_ZIP="$ROOT/thirdparty/STM32CubeF1-v1.8.7.zip"
DEV_ZIP="$ROOT/thirdparty/cmsis_device_f1-v4.3.5.zip"
DEST="$ROOT/thirdparty/STM32F1xx_CMSIS"
TMP="$ROOT/thirdparty/.cmsis_extract"

if [ ! -f "$CUBE_ZIP" ] || [ ! -f "$DEV_ZIP" ]; then
    echo "missing CMSIS zip files under thirdparty/" >&2
    exit 1
fi

rm -rf "$TMP" "$DEST"
mkdir -p "$TMP" "$DEST/Device/ST/STM32F1xx"

unzip -q "$CUBE_ZIP" 'STM32CubeF1-1.8.7/Drivers/CMSIS/Include/*' -d "$TMP"
unzip -q "$CUBE_ZIP" 'STM32CubeF1-1.8.7/Drivers/CMSIS/LICENSE.txt' -d "$TMP"
unzip -q "$DEV_ZIP" 'cmsis-device-f1-4.3.5/Include/*' -d "$TMP"
unzip -q "$DEV_ZIP" 'cmsis-device-f1-4.3.5/License.md' -d "$TMP"

cp -r "$TMP/STM32CubeF1-1.8.7/Drivers/CMSIS/Include" "$DEST/Include"
cp "$TMP/STM32CubeF1-1.8.7/Drivers/CMSIS/LICENSE.txt" "$DEST/LICENSE.txt"
cp -r "$TMP/cmsis-device-f1-4.3.5/Include" "$DEST/Device/ST/STM32F1xx/Include"
cp "$TMP/cmsis-device-f1-4.3.5/License.md" "$DEST/Device/ST/STM32F1xx/License.md"

cat > "$DEST/VERSION" <<'VERSION'
CMSIS core: STM32CubeF1 v1.8.7 (Drivers/CMSIS/Include)
STM32F1 device headers: STMicroelectronics/cmsis_device_f1 v4.3.5
VERSION

rm -rf "$TMP"
echo "extracted CMSIS to $DEST"
