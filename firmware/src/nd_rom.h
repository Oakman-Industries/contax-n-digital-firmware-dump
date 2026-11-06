/* Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0
 *
 * nd_rom.h — the few Contax N Digital firmware entry points used by nd_lastresort.c.
 *
 * Addresses are runtime virtual addresses (KSEG0) of the camera's own file API, identical in firmware 1.07 and 1.08.
 * They are only valid when called from inside the running firmware (RTOS up, CF card mounted) — never from reset.
 * Calling convention: integer args in a0..a3, return in v0 (gcc -mabi=eabi is compatible at these boundaries).
 */
#ifndef ND_ROM_H
#define ND_ROM_H

typedef int (*nd_open_fn)(const char *path, int flags, int mode);
typedef int (*nd_rw_fn)(int fd, void *buf, unsigned int len);
typedef int (*nd_close_fn)(int fd);

#define ND_OPEN   ((nd_open_fn)  0x8007D9BCu) /* open(path, flags, mode) -> fd, <0 on error */
#define ND_WRITE  ((nd_rw_fn)    0x8007E82Cu) /* write(fd, buf, len) -> bytes written (== len on success) */
#define ND_CLOSE  ((nd_close_fn) 0x8007FB50u) /* close(fd) -> <0 on error */

/* Size of the memory-mapped flash window that is dumped (base 0xBC000000). */
#ifndef ND_FLASH_SIZE
#define ND_FLASH_SIZE   0x00100000u           /* 1 MiB */
#endif

#endif /* ND_ROM_H */
