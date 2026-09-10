#!/usr/bin/env bash
set -euo pipefail

BODY_CONTROL_ROOT=/home/ubuntu/ros2ws/install/body_control
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

if [ "${EUID}" -ne 0 ]; then
  echo "Run with: sudo ./install_new_files.sh" >&2
  exit 1
fi

SOURCE_DRIVER="$SCRIPT_DIR/overlay/lib/libdev_inspire_gripper_can.so"
SOURCE_YAML="$SCRIPT_DIR/overlay/share/body_control/param/inspire_gripper.yaml"
TARGET_DRIVER="$BODY_CONTROL_ROOT/lib/libdev_inspire_gripper_can.so"
TARGET_YAML="$BODY_CONTROL_ROOT/share/body_control/param/inspire_gripper.yaml"

for REQUIRED_PATH in \
  "$SOURCE_DRIVER" \
  "$SOURCE_YAML" \
  "$BODY_CONTROL_ROOT/lib" \
  "$BODY_CONTROL_ROOT/share/body_control/param"; do
  if [ ! -e "$REQUIRED_PATH" ]; then
    echo "Missing required path: $REQUIRED_PATH" >&2
    exit 1
  fi
done

if [ -e "$TARGET_DRIVER" ]; then
  if [ "$(sha256sum "$TARGET_DRIVER" | awk '{print $1}')" != \
    "3baac3dcc85f634f008d4da62a08c055900df60f043b456aa1b17ff8742ecaf9" ]; then
    echo "Existing file differs; not replacing: $TARGET_DRIVER" >&2
    exit 1
  fi
  echo "Already present and correct: $TARGET_DRIVER"
else
  cp -a "$SOURCE_DRIVER" "$TARGET_DRIVER"
  echo "Added: $TARGET_DRIVER"
fi

if [ -e "$TARGET_YAML" ]; then
  if ! grep -qE '^left_gripper_installed: (true|false)$' "$TARGET_YAML" ||
     ! grep -qE '^right_gripper_installed: (true|false)$' "$TARGET_YAML"; then
    echo "Existing file is missing the v0.1.4 side switches; not replacing: $TARGET_YAML" >&2
    echo "Add left_gripper_installed and right_gripper_installed manually." >&2
    exit 1
  fi
  echo "Already present with v0.1.4 side switches; not modified: $TARGET_YAML"
else
  cp -a "$SOURCE_YAML" "$TARGET_YAML"
  echo "Added: $TARGET_YAML"
fi

echo "No existing configuration or launch file was modified."
echo "Use the files under examples/ as references for manual changes."
