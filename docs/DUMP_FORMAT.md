# `NDDUMP.BIN` format

Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0.

`NDDUMP.BIN` is always **1,048,604 bytes**: a 16-byte header, the 1 MiB flash image, and a 12-byte trailer. All
integers are **big-endian** 32-bit, matching the camera's CPU.

| File offset | Size | Field | Value |
|---|---|---|---|
| `0x000000` | 4 | magic 0 | `4E 44 4C 52` (`"NDLR"`) |
| `0x000004` | 4 | magic 1 | `44 55 4D 50` (`"DUMP"`) |
| `0x000008` | 4 | flash base address | `BC 00 00 00` |
| `0x00000C` | 4 | image length | `00 10 00 00` (1 MiB) |
| `0x000010` | 1,048,576 | flash image | the bytes read from `0xBC000000 … 0xBC0FFFFF` |
| `0x100010` | 4 | `crc_write` | CRC32 computed while writing the image |
| `0x100014` | 4 | `crc_reread` | CRC32 from a second, independent read of the flash |
| `0x100018` | 4 | length again | `00 10 00 00` |

Image offset *N* lives at file offset *N* + 16.

## Checking a dump

```bash
ls -l NDDUMP.BIN                        # 1048604
xxd -l 16 NDDUMP.BIN                    # 4e44 4c52 4455 4d50 bc00 0000 0010 0000  NDLRDUMP........
tail -c 12 NDDUMP.BIN | xxd             # crc_write == crc_reread, then 0010 0000
cmp -n 720912 NDDUMP_1.BIN NDDUMP_2.BIN # header + firmware region of two dumps must be identical
shasum -a 256 NDDUMP.BIN                # record this when sharing
```

- **`crc_write == crc_reread`** means the camera read the same flash contents twice in a row (a stable read).
- **Known quirk:** recomputing CRC32 (standard zlib/IEEE polynomial) over the 1 MiB image on a PC does **not** currently
  reproduce the stored value, even when the two stored values agree. The cause hasn't been found yet. Until it is,
  the reliable check is the two-dump comparison above. If you work it out, please open an issue.

## What's inside the image (as observed so far)

From the dumps of the one camera tested so far. Treat these as working notes, not a final map.

| Image offset | Contents | Changes between dumps? |
|---|---|---|
| `0x00000 – ~0xB0000` | Boot code + main firmware. After the first block, it sits 0xE8 bytes earlier than in the update file `CXBRFIRM.BIN`, so the flash layout is not a plain copy of the update file. | **No.** Byte-identical across every dump. |
| `~0xB0000 – 0xF8000` | Live camera state: real-time clock, counters, and a growing log of recent captures (file paths such as `A:\D_CONTAX\100CXBOX\KICX0016.JPG` with timestamps). | **Yes**, as you use the camera. |
| `0xF8000 – 0xF9FFF` | Parameter block A (settings / counters). | A few bytes (counters). |
| `0xFA000 – 0xFBFFF` | Parameter block B. All zero on the tested camera. | No |
| `0xFC000 – 0xFFFFF` | All zero on the tested camera. | No |

Open questions where your dumps help:

- Is the live-state area real flash, or battery-backed memory that shows up in the same address window? A dump taken
  right after a long time without any battery would help answer this.
- Does parameter block B ever hold data on other bodies?
- Is there any per-camera data (serial, calibration) anywhere in the image?
- Where does each menu setting live? Method: take two dumps back-to-back with **no photos taken in between**,
  changing exactly **one** setting between them. Apart from the ticking clock, the bytes that differ are that
  setting.
