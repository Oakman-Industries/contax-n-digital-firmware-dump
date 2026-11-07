# NDDUMP v1 — DO NOT INSTALL

Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0.

This folder keeps the exact v1 code (`nd_lastresort.c`, `nd_lastresort.bin`, SHA-256 `602d0b61…`) that was offered
here until 7 November 2026, so owners of cameras running v1 can see exactly what is installed. The full v1 firmware
image had SHA-256 `c8deab4c8822fbf050b885b9c1422619b73920058177e9a1b88f4d23e63f211d`.

v1 has two defects:

1. **No way back.** Without the marker, v1 returns without calling the camera's updater. A camera running v1 can
   never update its firmware from the menu again, not even back to stock.
2. **Dumps are mostly RAM.** v1 passes flash addresses straight to the card routine, which fills whole 512-byte
   sectors from the camera's RAM. Only 512 bytes in every 32 KiB of a v1 dump are real flash.

v2 (`../src/nd_dump_reversible.c`) fixes both. If your camera runs v1, see "If you installed v1" in the main README.
