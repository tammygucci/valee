#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# (Skiff XENO) The browser emulator without Emscripten: plain clang --target=wasm32 and wasm-ld (LLVM 15 or later).
# The page (worklet.js) instantiates the module with no imports, so the firmware's live code needs no C library: the
# test harness's libc calls are dead and --gc-sections drops them; web/emu/clang/include declares them so the sources
# compile, web/emu/clang/shim.c has the four the code does call (memcpy, memset, memmove, memcmp) and _initialize.
#   web/emu/build_clang.sh  ->  build/emu/{index.html, worklet.js, felucca.wasm, fonts/}, build/cast/, build/panel.js
# Run after one ./build.sh, or just tools/build.py's generate() (build/gen). The same outputs as web/emu/build.sh.
set -e
cd "$(dirname "$0")/../.."
[ -f build/gen/felucca_tables.h ] || { echo "emu: generate build/gen first (./build.sh, or python3 tools/build.py --gen)"; exit 1; }
CC=${CC:-clang}
LD=${WASM_LD:-wasm-ld}
VERSION=${EMU_VERSION:-$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)}   # (EMU_VERSION: a release's, vX.Y-suffix)
CFLAGS="--target=wasm32 ${EMU_OPT:--O2} -ffp-contract=off -std=gnu99 -w -nostdlib -ffreestanding -fvisibility=hidden \
 -isystem web/emu/clang/include -Ibuild/gen -Ifirmware/src"
LDFLAGS="--no-entry --gc-sections --export-dynamic -z stack-size=1048576 --initial-memory=33554432"
T=$(mktemp -d)
mkdir -p build/emu/fonts build/cast
$CC --target=wasm32 -O2 -fno-builtin -ffreestanding -nostdlib -fvisibility=hidden -c web/emu/clang/shim.c -o "$T/shim.o"
# shellcheck disable=SC2086
$CC $CFLAGS "-DFELUCCA_VERSION=\"$VERSION\"" -c web/emu/felucca_web.c -o "$T/felucca.o"
# shellcheck disable=SC2086
$LD $LDFLAGS -o build/emu/felucca.wasm "$T/felucca.o" "$T/shim.o"
# shellcheck disable=SC2086
$CC $CFLAGS "-DFELUCCA_VERSION=\"$VERSION\"" -DFELUCCA_CAST=1 -c web/emu/cast_follow.c -o "$T/cast.o"
# shellcheck disable=SC2086
$LD $LDFLAGS -o build/cast/felucca_cast.wasm "$T/cast.o" "$T/shim.o"
rm -rf "$T"
[ "${EMU_PREVIEW:-1}" = 0 ] && PREVIEW='' || PREVIEW=' (preview)'
sed -e "s/@VERSION@/$VERSION/g" -e "s/@PREVIEW@/$PREVIEW/g" web/emu/index.html > build/emu/index.html
cp web/emu/worklet.js build/emu/
cp web/panel.js build/
cp web/emu/fonts/DotGothic16-subset.woff web/emu/fonts/OFL.txt build/emu/fonts/
echo "emu (clang): build/emu ($(wc -c < build/emu/felucca.wasm | tr -d ' ') B wasm, $VERSION)"
