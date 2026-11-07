#!/usr/bin/env python3
# Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0
"""check_nddump.py — check an NDDUMP.BIN made by NDDUMP v2. Python 3 standard library only; reads the file, writes nothing.

PASS needs: the right size, a v2 header ('NDLR' 'DMP2'), and three equal CRC32 values: the two the camera stored
(crc_write, crc_reread) and one computed here over the image. v1 files ('NDLR' 'DUMP') are reported as not being
flash copies (see docs/DUMP_FORMAT.md).

Usage: python3 tools/check_nddump.py NDDUMP.BIN [MORE.BIN ...]
"""
import struct
import sys
import zlib
from pathlib import Path

SIZE = 16 + 0x100000 + 12


def check(path):
    data = Path(path).read_bytes()
    rows = []

    def row(name, ok, detail=""):
        rows.append((name, ok, detail))
        return ok

    if not row("size is 1,048,604 bytes", len(data) == SIZE, f"{len(data):,} bytes"):
        return rows
    magic, base, length = data[:8], *struct.unpack(">II", data[8:16])
    if magic == b"NDLRDUMP":
        row("format v2", False, "this is a v1 dump: most of it was copied from the camera's RAM, not its flash, "
                                "so it is not a flash backup (see docs/DUMP_FORMAT.md)")
        return rows
    row("format v2 (header NDLR DMP2)", magic == b"NDLRDMP2", magic.decode("latin-1"))
    row("base 0xBC000000, length 1 MiB", base == 0xBC000000 and length == 0x100000, f"{base:#010x}, {length:#x}")
    crc_w, crc_r, length2 = struct.unpack(">III", data[-12:])
    here = zlib.crc32(data[16:16 + 0x100000]) & 0xFFFFFFFF
    row("trailer length matches", length2 == length, f"{length2:#x}")
    row("stable read (crc_write == crc_reread)", crc_w == crc_r, f"{crc_w:08x} / {crc_r:08x}")
    row("intact copy (CRC computed here == stored)", here == crc_w, f"{here:08x}")
    return rows


def main(argv):
    if not argv:
        print(__doc__.strip().splitlines()[-1])
        return 2
    all_ok = True
    for path in argv:
        rows = check(path)
        ok = all(r[1] for r in rows)
        all_ok &= ok
        print(f"{path}:")
        for name, good, detail in rows:
            print(f"  [{'PASS' if good else 'FAIL'}] {name:<44} {detail}")
        print(f"  => {'PASS' if ok else 'FAIL'}\n")
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
