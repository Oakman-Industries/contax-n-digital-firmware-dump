<p align="center">
  <img src="docs/banner.png" alt="NDDUMP — Contax N Digital firmware &amp; flash dump to a CF card" width="100%">
</p>

# Contax N Digital — firmware & flash dump (NDDUMP)

**© 2026 Oakman Industries — licensed CC BY-NC-SA 4.0** (build on it, keep it open & non-commercial; see `LICENSE`).

A tiny custom firmware for the **Contax N Digital** that adds a **read-only "dump my camera's flash to the CF card"**
function to the camera's *Firmware Update* command. With a marker file on the card, the command writes a 1 MiB copy of
the camera's flash to `NDDUMP.BIN`. Without the marker, the command updates firmware exactly as the stock firmware
does, so you can always go back to stock.

> [!CAUTION]
> **NDDUMP v1, the version published here until 7 November 2026, had two serious defects. Don't install it.**
> Cameras running v1 can never update their firmware from the menu again, and v1 dumps are mostly copies of the
> camera's RAM, not its flash. If you installed v1, read **[If you installed v1](#if-you-installed-v1)**.
> This page now describes **v2**, which fixes both.

> [!WARNING]
> **Installing any custom firmware can permanently brick your camera.** The install goes through the camera's own
> stock updater, which erases and rewrites the flash. If power drops or the 20-year-old flash fails during that step,
> the camera is gone. There is no recovery without opening the camera and attaching a flash programmer.
>
> **v2 has been tested in emulation only. Nobody has installed it on a real camera yet.** It changes the same two flash
> blocks as v1, which installed and ran on one real camera, but its new code has never run on hardware. If you install
> it, you are the first. Read the whole guide and decide for yourself.

> **❤️ Support more Contax N Digital work:** this is part of an ongoing effort to reverse-engineer and preserve the
> Contax N Digital (backups, calibration recovery, an open RAW developer). If it's useful to you, please consider
> supporting the work on **[Ko-Fi](https://ko-fi.com/oakman)**. Donations go directly toward hardware, testing and
> further research.

---

## Why this exists

The Contax N Digital (2002) is long discontinued. No service support remains, and every year more bodies fail. The
camera keeps its firmware and stored settings in on-board flash. There is no other way to get a copy out: FireWire
only transfers images, and the camera's internal serial diagnostics aren't exposed on the body. When a body dies, its
flash contents die with it.

NDDUMP copies the flash onto an ordinary CF card, so that:

- **you** have a backup of your own camera's flash, and
- **the community** can compare dumps across bodies, to find out what is per-camera and what is common, where the
  settings live, and whether other firmware revisions ever existed.

## If you installed v1

v1 is the firmware that was offered here until 7 November 2026 (SHA-256 `c8deab4c…e63f211d`, dump header
`NDLRDUMP`). If your camera runs it:

- **Your camera still works as a camera.** Apart from the update command, it runs the stock 1.08 firmware.
- **The update command can never flash again.** v1 never calls the camera's updater, so you can't go back to stock or
  install v2 from the menu. Nothing you put on a card changes this: all v1 looks at on the card is whether
  `NDDUMP.RUN` and `NDDUMP.BIN` exist.
- **Your v1 dumps are not flash backups.** v1 passed flash addresses straight to the camera's card-writing routine.
  On a real camera, that routine copied every whole 512-byte sector from the camera's RAM instead of its flash. Only
  the partial sectors, 512 bytes in every 32 KiB, are real flash. Keep the files if you like, but don't rely on them.
  `tools/check_nddump.py` identifies them.
- **The way back is hardware.** v1 changed only two 64 KiB flash blocks: block 3 (offset `0x30000`) and block 10
  (`0xA0000`). Every real-flash sample in the v1 dumps matches the update file at the same offset, so the stock bytes
  for those blocks are bytes `0x30000–0x3FFFF` and `0xA0000–0xAFFFF` of the stock 1.08 `CXBRFIRM.BIN`. Someone with a
  flash programmer would remove the chip, **read the whole chip first** (that read is your real flash backup), rewrite
  those two blocks, check them, and refit the chip. The settings blocks don't need to be touched. The chip's part
  number isn't documented yet.

Please open an [install report](../../issues/new?template=install-report.yml) saying you have v1, so we know how many
cameras are affected.

## What changed in v2

| | v1 (don't install) | v2 |
|---|---|---|
| Update command, **no** `NDDUMP.RUN` on the card | does nothing; the camera can never update again | runs the camera's own, unmodified updater, so updating and going back to stock work as normal |
| Update command, `NDDUMP.RUN` on the card | dumps to `NDDUMP.BIN`; no flashing | dumps to `NDDUMP.BIN`; no flashing |
| How flash gets to the card | flash addresses passed to the card routine; whole sectors came from RAM | flash copied into a 512-byte RAM buffer first; only that buffer goes to the card |
| CRC check on your computer | never matched | must match; a mismatch means a bad dump |
| Dump header | `NDLRDUMP` | `NDLRDMP2` |

## What's in this repo

| File | What it is |
|---|---|
| `firmware/nddump-v2-fw108.ips` | The v2 patch: turns a stock **1.08** `CXBRFIRM.BIN` into NDDUMP v2. Contains only the changed bytes. |
| `firmware/nd_dump_reversible.bin` | The compiled v2 code (936 bytes), if you'd rather build the firmware by hand with `dd`. |
| `firmware/src/` | Full C source, linker script and build script for the v2 code (and v1, for reference). |
| `firmware/stock-1.08/CXBRFIRM.BIN` | The original, unmodified 1.08 firmware (© Kyocera Corporation, all rights retained). You need it to build v2 and to go back to stock. |
| `firmware/legacy-v1/` | The v1 source and binary, kept only so v1 owners can see exactly what's on their camera. **Don't install.** |
| `firmware/SHA256SUMS` | Hashes of the stock firmware, the v2 output, the patch and the code blobs. |
| `tools/check_nddump.py` | Checks a dump (Python 3, no extra packages). |
| `docs/DUMP_FORMAT.md` | Exactly what is inside `NDDUMP.BIN`, and why v1 dumps are mostly RAM. |

There is deliberately no ready-made firmware image any more: you build it in Step 1 and check its hash.

## Requirements

- A **Contax N Digital** that powers on, takes pictures and reads CF cards.
- A **fully charged battery and the AC adapter** (use both during the install).
- A **CF card the camera reads reliably**, formatted in the camera, and a card reader.
- A computer with `shasum`/`sha256sum` (built into macOS and Linux; on Windows use WSL or Git Bash, or
  `certutil -hashfile <file> SHA256`), plus `python3` to check dumps.

---

## Step-by-step

### Step 1 — Build the v2 firmware and check its hash

Download this repository (green **Code** button → *Download ZIP*, or `git clone`). Start from the original 1.08
firmware: either `firmware/stock-1.08/CXBRFIRM.BIN` from this repo, or your own copy (the official Windows update
`Firm0108w.EXE` is a self-extracting archive; `tar -xf Firm0108w.EXE` unpacks it without running it and gives
`D_contax/100cxbox/CXBRFIRM.BIN`). Check it:

```bash
shasum -a 256 firmware/stock-1.08/CXBRFIRM.BIN
# must be: ea97198569531a8cf9d0e3f3e22c88b2e209e9dac3773daff33d1a698f635dfd
```

Then apply `firmware/nddump-v2-fw108.ips` with any IPS patcher (e.g. Floating IPS), **or** write the two 4-byte call
redirects and the 936-byte code with `dd`:

```bash
cp firmware/stock-1.08/CXBRFIRM.BIN CXBRFIRM.BIN
printf '\x0c\x02\x8b\x72' | dd of=CXBRFIRM.BIN bs=1 seek=$((0x386b0)) conv=notrunc
printf '\x0c\x02\x8b\x72' | dd of=CXBRFIRM.BIN bs=1 seek=$((0x38718)) conv=notrunc
dd if=firmware/nd_dump_reversible.bin of=CXBRFIRM.BIN bs=1 seek=$((0xa2eb0)) conv=notrunc
```

**This is the most important check in the whole guide:**

```bash
shasum -a 256 CXBRFIRM.BIN
# must be: 43eefee09a3ec052a16dd40a192d85360121351c4834aaedb43002770a75347f   (746,540 bytes)
```

If it doesn't match, **stop**. Never flash a file whose hash you haven't confirmed.

### Step 2 — Prepare the install card

Format the card **in the camera**, then put the firmware you just built at exactly this path. **Don't put
`NDDUMP.RUN` on this card.**

```
CF card (A:)
└── D_CONTAX
    └── 100CXBOX
        └── CXBRFIRM.BIN        ← NDDUMP v2 (sha256 43eefee0…)
```

Check the hash again on the card before you go on.

### Step 3 — Install (the risky step)

1. Insert the full battery **and** connect the AC adapter.
2. Insert the card and run the camera's firmware update from the menu (**UPDATE SOFTWARE?**). Confirm it.
3. **Do not touch anything** until the update has finished and the camera has restarted: no buttons, no card removal,
   no power interruption.

The camera now runs 1.08 + NDDUMP v2. It shoots, reviews and transfers images as before. Its update command still
updates normally when there's no `NDDUMP.RUN` on the card.

### Step 4 — Take a dump (read-only)

1. Prepare a card with an **empty file named `NDDUMP.RUN` in the root**, and no old `NDDUMP.BIN`:

   ```
   CF card (A:)
   └── NDDUMP.RUN               ← empty file (0 bytes)
   ```

   If your camera only offers the update when a firmware file is on the card, also copy **the same NDDUMP v2 file you
   installed** (sha256 `43eefee0…`) to `D_CONTAX/100CXBOX/CXBRFIRM.BIN`. Never use the stock file, or an empty or dummy
   file, on a dump card. The reason: if the camera ever failed to read `NDDUMP.RUN`, v2 would behave like stock and
   install whatever `CXBRFIRM.BIN` is there. With the same v2 file that just re-installs what you already have. (Please
   tell us whether your camera needed the file; it isn't documented yet.)
2. Use good power, insert the card and run the firmware update from the menu.
3. Because `NDDUMP.RUN` is on the card, the firmware writes the flash to **`A:\NDDUMP.BIN`** instead of updating. It
   does not flash anything. v2 writes in small pieces, so it may take a while. Wait until card activity has stopped.
4. Copy `NDDUMP.BIN` to your computer. It should be exactly **1,048,604 bytes**.

If an `NDDUMP.BIN` already exists, the dump is refused and nothing happens. Rename or move the old one off the card
first (e.g. `NDDUMP_1.BIN`), then dump again.

### Step 5 — Check the dump

```bash
python3 tools/check_nddump.py NDDUMP.BIN
```

A good dump shows `PASS`: header `NDLRDMP2`, and three equal CRC32 values (the two the camera stored and one computed
on your computer). If the computed CRC doesn't match, the copy is bad: take another dump, and please report it. Taking
two dumps and checking both is a good habit.

### Step 6 — Keep it safe, and share it

Keep several copies of your verified dumps in different places. Then please [share it](#share-your-dump).

### Going back to stock

Put the stock 1.08 file (sha256 `ea97…f635dfd`) on a card as `D_CONTAX/100CXBOX/CXBRFIRM.BIN`, with **no**
`NDDUMP.RUN`, and run the firmware update as in Step 3. This is a normal firmware update with the normal risk, so use
good power and don't interrupt it. It rewrites every byte NDDUMP changed.

---

## How it works (brief)

- The stock firmware's update command calls one update routine from two places. NDDUMP rewrites those two calls
  (file offsets `0x386B0` and `0x38718`) to point at a small gate routine. The gate lives in an unused, all-zero region
  of the stock image (file offset `0xA2EB0`, runtime address `0x800A2DC8`). Nothing else in the firmware changes.
- The **gate** checks for `A:\NDDUMP.RUN`. If it's present, it calls the dumper. If not, it jumps to the camera's
  original update routine with the same argument, so the update behaves exactly as stock.
- The **dumper** reads the 1 MiB flash window at `0xBC000000` with 16-bit reads into a 512-byte RAM buffer, writes
  that buffer to a new `A:\NDDUMP.BIN` using the camera's own file routines, and repeats. It never sends a flash
  erase/program command. The file has a header, a CRC32 of exactly the bytes written, and a second CRC32 from a full
  re-read.
- The full source is `firmware/src/nd_dump_reversible.c` (about 120 lines of C). `firmware/src/build.sh` rebuilds the
  blob with a MIPS cross-compiler (`gcc-mips-linux-gnu`) and prints the released hashes to compare. A different
  compiler version may not produce a byte-identical blob.

## Accuracy & limitations

Honest status. Please help close these gaps.

- **v2 is tested in emulation only.** The real compiled code was run against the stock firmware in an emulator:
  - the dump is byte-exact at full size, never writes flash, and only ever hands RAM to the card routine;
  - without the marker, the stock updater is entered in exactly the same state as on stock firmware;
  - the patched firmware boots the same way as stock;
  - a model of the card behaviour that broke v1 reproduces the v1 fault and doesn't affect v2.

  What emulation can't show is the real card routine writing from v2's RAM buffer. The CRC check in Step 5 catches
  it if that goes wrong.
- **The install risk is real and can't be engineered away here.** The stock updater erases and rewrites flash,
  including the boot area, and nothing can protect against a power or hardware failure during that. Going back to
  stock carries the same risk as any firmware update.
- **Never put a dummy or empty `CXBRFIRM.BIN` on a card.** The stock updater doesn't check the file it is given, and
  from our reading of its code an empty file would still erase and rewrite the first flash block.
- **Only a 1.08-based build is provided.** Installing it replaces the whole firmware, so a camera on 1.07 ends up on
  1.08 + NDDUMP. This hasn't been tested on a body that started on 1.07.
- **"1 MiB" is the window we dump, not a confirmed chip size.** Some parts of the camera (e.g. the body/lens
  controllers) have their own chips, which this does **not** dump.
- **Restoring a dump to a camera is not supported.** Writing flash back needs a flash programmer.
- **What's in the flash is mostly unknown again.** Earlier notes here described live state, a capture log and an
  empty "calibration" block. Those came from v1 dumps, which were mostly RAM, so they are withdrawn. What still holds,
  from the real-flash parts of v1 dumps: the firmware area is a plain copy of the update file, and the block at
  `0xF8000` holds a counter that changes as photos are taken. Whether any CCD calibration is stored in the camera is
  open again. If your CCD Parameter Disc is lost, see our
  [ccd-disc-recreator](https://github.com/Oakman-Industries/ccd-disc-recreator).

## Share your dump

Dumps from many bodies are what make this valuable for preservation. If you've made a **v2** dump, please
**[open a "Share a dump" issue](../../issues/new?template=share-a-dump.yml)** with:

- the SHA-256 of each `NDDUMP.BIN`, and the `tools/check_nddump.py` output;
- the firmware version the camera showed **before** you installed NDDUMP;
- anything notable: what the screen showed, how long it took, any errors;
- a **download link** to the dump(s) hosted wherever you like (a cloud drive or archive). **Please don't commit dumps
  into this repository.**

**Privacy:** a v2 dump is a copy of your camera's flash: its firmware and whatever settings and per-camera data the
camera stores there. We don't know yet exactly what that includes. Only share what you're comfortable with. Serial
numbers are optional.

Even if you can't share the file, posting the hashes and your experience (including a failed install) is valuable.

## Contributing

See **[CONTRIBUTING.md](CONTRIBUTING.md)**. Contributions are released under the same **CC BY-NC-SA 4.0** license.
Keep your findings open and non-commercial. Apart from the 1.08 file listed in `NOTICE`, please don't submit
Kyocera/Contax firmware or software to this repo. Share dumps by link.

## Legal / ethical

By **Oakman Industries**, for owners preserving their **own** discontinued cameras. This is interoperability and
preservation work for long-abandoned hardware.

**Original firmware:** `firmware/stock-1.08/CXBRFIRM.BIN` is the Contax N Digital 1.08 firmware, **copyright Kyocera
Corporation, which retains all rights to it**. It is included only so owners can back up and preserve their own
cameras. It is **not** licensed under CC BY-NC-SA 4.0, and Oakman Industries claims no ownership of it and is not
affiliated with or endorsed by Kyocera Corporation. Full details are in [`NOTICE`](NOTICE). Rights holders with a
concern, please open an issue.

You use this entirely at your own risk; see the warnings at the top and the disclaimer in `LICENSE`.

## Support

❤️ **Support the project:** [ko-fi.com/oakman](https://ko-fi.com/oakman)

NDDUMP is one piece of a larger Contax N Digital preservation project (firmware backup, calibration recovery, and an
open RAW developer). Contributions fund the hardware, test cameras, media and time that keep it going, and help bring
more of these cameras back to life.

## License

**CC BY-NC-SA 4.0** (Oakman Industries' own work only; the Kyocera firmware file is excluded, see `NOTICE`).
Copyright © 2026 **Oakman Industries**. You may use, study, modify and build on this work (code
*and* the research/findings it embodies), but you must: **credit Oakman Industries**, **release your derivatives and
your own findings openly under the same license**, and **not use it commercially**. Full text in `LICENSE`.
(Deliberately copyleft + non-commercial, so improvements stay open to the community. Because of the NonCommercial
clause this is intentionally not an OSI "open source" license.)
