#!/usr/bin/env bash
# Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0
#
# build.sh — rebuild the NDDUMP code blobs from source (optional; the prebuilt .bin files are the released ones).
# Needs a big-endian-capable MIPS cross toolchain, e.g. Debian/Ubuntu `gcc-mips-linux-gnu`.
# A different compiler version may produce a different (but equivalent) binary; compare the hashes below.
#
# Usage: firmware/src/build.sh [OUT_DIR]      (default OUT_DIR: ./build)
set -euo pipefail
SRC="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-build}"
CROSS="${CROSS:-mips-linux-gnu-}"
mkdir -p "$OUT"

CFLAGS=(-O2 -EB -mabi=eabi -march=r3000 -msoft-float -mno-abicalls -fno-pic -G0
        -ffreestanding -fno-builtin -fno-jump-tables -falign-functions=4 -I "$SRC")

sha() { if command -v sha256sum >/dev/null; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi; }

build() {  # build <source.c> <entry symbol> <name>
    "${CROSS}gcc" "${CFLAGS[@]}" -c "$1" -o "$OUT/$3.o"
    "${CROSS}gcc" -EB -mabi=eabi -nostdlib -Wl,--build-id=none -Wl,--defsym,CAVE_VA=0x800A2DC8 -Wl,-e,"$2" \
          -T "$SRC/nd.ld" "$OUT/$3.o" -o "$OUT/$3.elf"
    "${CROSS}objcopy" -O binary --only-section=.text --only-section=.rodata "$OUT/$3.elf" "$OUT/$3.bin"
    echo "$3.bin: $(wc -c < "$OUT/$3.bin" | tr -d ' ') bytes  sha256 $(sha "$OUT/$3.bin")"
}

build "$SRC/nd_dump_reversible.c"          nd_dump_gate       nd_dump_reversible
echo "  released v2 blob:  936 bytes  sha256 e5d318842c8d8f686bb7325309a0802fef6cf1eac7629c81c9dff210e3b3bb5c"
build "$SRC/../legacy-v1/nd_lastresort.c"  nd_lastresort_gate nd_lastresort
echo "  v1 (DO NOT INSTALL): 776 bytes  sha256 602d0b61b280a97829bf031f3c17f2419165307b6710d78498853ce89cd00859"
