#!/usr/bin/env bash
# Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0
#
# build.sh — rebuild firmware/nd_lastresort.bin from source (optional; the prebuilt .bin is the exact one that was
# flashed and tested). Needs a big-endian-capable MIPS cross toolchain, e.g. Debian/Ubuntu `gcc-mips-linux-gnu`.
#
# Usage: firmware/src/build.sh [OUT_DIR]      (default OUT_DIR: ./build)
set -euo pipefail
SRC="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-build}"
CROSS="${CROSS:-mips-linux-gnu-}"
mkdir -p "$OUT"

CFLAGS=(-O2 -EB -mabi=eabi -march=r3000 -msoft-float -mno-abicalls -fno-pic -G0
        -ffreestanding -fno-builtin -fno-jump-tables -falign-functions=4 -I "$SRC")
"${CROSS}gcc" "${CFLAGS[@]}" -c "$SRC/nd_lastresort.c" -o "$OUT/nd_lastresort.o"
"${CROSS}gcc" -EB -mabi=eabi -nostdlib -Wl,--build-id=none -Wl,--defsym,CAVE_VA=0x800A2DC8 \
      -T "$SRC/nd.ld" "$OUT/nd_lastresort.o" -o "$OUT/nd_lastresort.elf"
"${CROSS}objcopy" -O binary --only-section=.text --only-section=.rodata \
      "$OUT/nd_lastresort.elf" "$OUT/nd_lastresort.bin"

if command -v sha256sum >/dev/null; then SUM=$(sha256sum "$OUT/nd_lastresort.bin" | cut -d' ' -f1)
else SUM=$(shasum -a 256 "$OUT/nd_lastresort.bin" | cut -d' ' -f1); fi
echo "nd_lastresort.bin: $(wc -c < "$OUT/nd_lastresort.bin" | tr -d ' ') bytes  sha256 $SUM"
echo "released blob:     776 bytes  sha256 602d0b61b280a97829bf031f3c17f2419165307b6710d78498853ce89cd00859"
