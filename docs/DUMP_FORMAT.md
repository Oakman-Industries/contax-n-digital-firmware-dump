# `NDDUMP.BIN` format

Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0.

`NDDUMP.BIN` is always **1,048,604 bytes**: a 16-byte header, the 1 MiB flash image, and a 12-byte trailer. All
integers are **big-endian** 32-bit, matching the camera's CPU.

| File offset | Size | Field | Value |
|---|---|---|---|
| `0x000000` | 4 | magic 0 | `4E 44 4C 52` (`"NDLR"`) |
| `0x000004` | 4 | magic 1 | `44 4D 50 32` (`"DMP2"`, v2). v1 files have `44 55 4D 50` (`"DUMP"`), see below. |
| `0x000008` | 4 | flash base address | `BC 00 00 00` |
| `0x00000C` | 4 | image length | `00 10 00 00` (1 MiB) |
| `0x000010` | 1,048,576 | flash image | the flash window `0xBC000000 … 0xBC0FFFFF` |
| `0x100010` | 4 | `crc_write` | CRC32 of exactly the image bytes written to the card |
| `0x100014` | 4 | `crc_reread` | CRC32 from a second full read of the flash |
| `0x100018` | 4 | length again | `00 10 00 00` |

Image offset *N* lives at file offset *N* + 16. The CRC is the standard CRC32 (zlib/IEEE polynomial).

## Checking a dump

```bash
python3 tools/check_nddump.py NDDUMP.BIN
```

A v2 dump passes when all three CRC values agree:

- **`crc_write == crc_reread`**: the camera read the same flash contents twice (a stable read).
- **CRC computed on your computer == `crc_write`**: the file on your computer holds exactly the bytes the camera read.

By hand: `xxd -l 16 NDDUMP.BIN` must show `NDLRDMP2`, and `tail -c 12 NDDUMP.BIN | xxd` shows the two stored CRCs.

## Why v1 dumps are mostly RAM

v1 (header `NDLRDUMP`) passed flash addresses straight to the camera's card-writing routine, 32 KiB at a time after a
16-byte header. The camera's card routine evidently handles each write in two ways:

- **Partial sectors** (the first 496 bytes and the last 16 bytes of every 32 KiB write) came out as real flash. They
  appear to be copied by the CPU.
- **Whole 512-byte sectors** appear to be transferred from the address directly. On the camera, the flash address
  used there reaches the camera's RAM instead, so these sectors came out as RAM.

Every v1 dump examined fits this exactly: the partial sectors match the installed firmware file byte-for-byte, and the
whole sectors match the firmware's working copy in RAM. That also explains the old "known quirk": the camera computed
its CRCs over real flash, but different (RAM) bytes reached the card, so a CRC computed on a computer could never match.
v2 copies flash into a RAM buffer first, so this can't happen.

## What's inside the image

Earlier versions of this page mapped live state, a capture log and an "empty" parameter block. Those observations came
from v1 dumps, so they described RAM and are withdrawn. What is known from the real-flash parts of v1 dumps:

| Image offset | Contents |
|---|---|
| `0x00000 – 0xB642B` | The firmware: a byte-for-byte copy of the installed `CXBRFIRM.BIN` from offset 0. |
| `0xF8000 – …` | A block written by the camera itself. It includes a counter that changes as photos are taken. |
| everything else | Unknown until v2 dumps exist. |

Open questions where your v2 dumps help:

- What do the blocks at `0xF8000` and `0xFA000` hold, and do they differ between bodies?
- Is there any per-camera data (serial, calibration) anywhere in the image?
- Where does each menu setting live? Method: take two dumps back-to-back with **no photos taken in between**, changing
  exactly **one** setting between them. The bytes that differ belong to that setting.
