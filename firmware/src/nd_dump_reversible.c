/* Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0
 *
 * nd_dump_reversible.c — NDDUMP v2: read-only flash dump-to-CF for the Contax N Digital, with the stock firmware
 * update kept intact.
 *
 * This routine is spliced into an unused, all-zero region of the stock 1.08 firmware. The two calls to the
 * firmware-update routine (0x80036538) are redirected to nd_dump_gate(). After installation, the firmware update
 * command does:
 *
 *   - marker file A:\NDDUMP.RUN present -> copy the 1 MiB flash window (memory-mapped at 0xBC000000) to a NEW file
 *     A:\NDDUMP.BIN, with a 16-byte header and a 12-byte CRC trailer, then return. Nothing is flashed.
 *   - marker absent -> jump to the camera's own, unmodified update routine with the same argument, so the update
 *     works exactly as on stock firmware. To remove NDDUMP, run a normal update with the stock CXBRFIRM.BIN.
 *
 * Changes from v1 (nd_lastresort.c, do not install):
 *   - v1 returned without calling the updater when there was no marker, so a v1 camera can never be updated again
 *     from the menu. v2 hands over to the stock updater instead.
 *   - v1 passed flash addresses straight to the card write routine. On a real camera, whole 512-byte sectors were then
 *     copied from RAM, not flash. v2 first copies flash into a small RAM buffer with 16-bit reads, and only that buffer
 *     is ever written to the card. The CRC is computed over exactly those bytes, so a CRC recomputed on a computer
 *     must match the stored value.
 *
 * Properties:
 *   - Read-only: flash is read as ordinary memory; no flash erase/program commands are ever issued by this code.
 *   - No overwrite: if A:\NDDUMP.BIN can already be opened, the dump is refused (-1). Rename/move old dumps first.
 *   - Dual CRC: crc_write covers the bytes written; crc_reread comes from a second full read through the same path.
 *   - Fail-safe: any open/write/close error stops immediately with a distinct negative code (-1..-6).
 *
 * WARNING: installing this firmware uses the camera's stock updater, which erases and reprograms flash. An interrupted
 * or failed install can permanently brick the camera. See README.md.
 */
#include "nd_rom.h"

#define ND_DR_MAGIC0 0x4E444C52u /* "NDLR" */
#define ND_DR_MAGIC1 0x444D5032u /* "DMP2": format v2 */
#define ND_DR_BOUNCE 0x200u      /* 512-byte RAM buffer, smaller than the stock update routine's own stack frame */

/* The camera's own, unmodified firmware-update routine. Reached only when there is no marker, by a tail call. */
typedef int (*nd_worker_fn)(void *arg);
#define ND_UPDATE_WORKER ((nd_worker_fn)0x80036538u)

static unsigned int crc32_update(unsigned int crc, const unsigned char *p, unsigned int n)
{
    crc = ~crc;
    while (n--) {
        crc ^= *p++;
        for (int k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320u & (unsigned int)(-(int)(crc & 1u)));
    }
    return ~crc;
}

static int write_all(int fd, const void *buf, unsigned int n)
{
    return (ND_WRITE(fd, (void *)buf, n) == (int)n) ? 0 : -1;
}

/* Copy flash into RAM using 16-bit reads. n must be even. */
static void flash_to_ram(unsigned short *dst, unsigned int flash_addr, unsigned int n)
{
    const volatile unsigned short *src = (const volatile unsigned short *)flash_addr;
    for (unsigned int i = 0; i < n / 2u; i++)
        dst[i] = src[i];
}

/* Read-only flash dump to a NEW card file. Every byte given to write() comes from the RAM buffer. */
__attribute__((noinline))
int nd_dump_nor(void)
{
    const unsigned int base = 0xBC000000u;
    const unsigned int len  = ND_FLASH_SIZE;
    unsigned short bounce[ND_DR_BOUNCE / 2u];
    unsigned int *w = (unsigned int *)bounce;

    /* No overwrite: if the output opens read-only, it already exists. */
    int ex = ND_OPEN("A:\\NDDUMP.BIN", 0x8000, 0x80);
    if (ex >= 0) { ND_CLOSE(ex); return -1; }

    int fd = ND_OPEN("A:\\NDDUMP.BIN", 0x8102, 0x180); /* create + write, new file */
    if (fd < 0) return -2;

    w[0] = ND_DR_MAGIC0; w[1] = ND_DR_MAGIC1; w[2] = base; w[3] = len;
    if (write_all(fd, bounce, 16u)) { ND_CLOSE(fd); return -3; }

    unsigned int crc = 0, off;
    for (off = 0; off < len; off += ND_DR_BOUNCE) {
        unsigned int n = len - off;
        if (n > ND_DR_BOUNCE) n = ND_DR_BOUNCE;
        flash_to_ram(bounce, base + off, n);
        crc = crc32_update(crc, (const unsigned char *)bounce, n); /* CRC of exactly what goes to the card */
        if (write_all(fd, bounce, n)) { ND_CLOSE(fd); return -4; }
    }

    /* Second pass through the same read path: crc2 == crc means the flash read was stable. */
    unsigned int crc2 = 0;
    for (off = 0; off < len; off += ND_DR_BOUNCE) {
        unsigned int n = len - off;
        if (n > ND_DR_BOUNCE) n = ND_DR_BOUNCE;
        flash_to_ram(bounce, base + off, n);
        crc2 = crc32_update(crc2, (const unsigned char *)bounce, n);
    }

    w[0] = crc; w[1] = crc2; w[2] = len;
    if (write_all(fd, bounce, 12u)) { ND_CLOSE(fd); return -5; }

    return (ND_CLOSE(fd) < 0) ? -6 : 0;
}

/* Entry point, placed first in the blob. Marker present -> dump, never flash. No marker -> stock update. */
__attribute__((section(".text.entry")))
int nd_dump_gate(void *arg)
{
    int fd = ND_OPEN("A:\\NDDUMP.RUN", 0x8000, 0x80); /* read-only marker probe */
    if (fd >= 0) {
        ND_CLOSE(fd);
        return nd_dump_nor();
    }
    return ND_UPDATE_WORKER(arg);        /* no marker: the camera's own update routine, same argument */
}
