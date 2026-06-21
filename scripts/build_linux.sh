#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON_BIN="${PYTHON:-python3}"
MMSOURCE="${MMSOURCE:-/tmp/mmsource-2.0}"
HL2SDK_ROOT="${HL2SDK_ROOT:-/tmp/hl2sdk-root}"
HL2SDK_MANIFESTS="${HL2SDK_MANIFESTS:-/tmp/hl2sdk-manifests}"
PROTOC="${PROTOC:-$HL2SDK_ROOT/hl2sdk-cs2/devtools/bin/linux/protoc}"

mkdir -p "$ROOT/generated"
"$PROTOC" \
  --proto_path="$HL2SDK_ROOT/hl2sdk-cs2/common" \
  -I="$HL2SDK_ROOT/hl2sdk-cs2/thirdparty/protobuf-3.21.8/src" \
  --cpp_out="$ROOT/generated" \
  "$HL2SDK_ROOT/hl2sdk-cs2/common/network_connection.proto"
"$PROTOC" \
  --proto_path="$HL2SDK_ROOT/hl2sdk-cs2/common" \
  -I="$HL2SDK_ROOT/hl2sdk-cs2/thirdparty/protobuf-3.21.8/src" \
  --cpp_out="$ROOT/generated" \
  "$HL2SDK_ROOT/hl2sdk-cs2/common/networkbasetypes.proto"

mkdir -p "$ROOT/build_linux"
cd "$ROOT/build_linux"
"$PYTHON_BIN" ../configure.py \
  --sdks cs2 \
  --targets x86_64 \
  --mms_path "$MMSOURCE" \
  --hl2sdk-root "$HL2SDK_ROOT" \
  --hl2sdk-manifests "$HL2SDK_MANIFESTS"
ambuild
