#!/usr/bin/env bash
set -euo pipefail
teleport_logistics_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
teleport_logistics_binary="$(mktemp /tmp/teleport-logistics-tests.XXXXXX)"
trap 'rm -f "$teleport_logistics_binary"' EXIT
bash -n "$teleport_logistics_root/scripts/build-linux.sh" "$teleport_logistics_root/scripts/doctor-linux.sh" \
    "$teleport_logistics_root/scripts/setup-wwise.sh" "$teleport_logistics_root/scripts/test.sh"
"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Werror -pedantic \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$teleport_logistics_root/TeleportLogistics/Source/TeleportLogistics/Public" \
    "$teleport_logistics_root/tests/transport_test.cpp" -o "$teleport_logistics_binary"
# LeakSanitizer needs ptrace; overridable via ASAN_OPTIONS.
# Address/undefined-behaviour instrumentation remains enabled.
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$teleport_logistics_binary"
python3 "$teleport_logistics_root/scripts/validate.py"
python3 "$teleport_logistics_root/scripts/validate-model-alignment.py"
python3 "$teleport_logistics_root/scripts/validate-screen-atlas.py"
python3 "$teleport_logistics_root/scripts/validate-style-assets.py"

python3 -m unittest discover -s "$teleport_logistics_root/tests" -p "test_*.py"
