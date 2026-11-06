/* Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0
 *
 * nd_lastresort.c — NDDUMP: read-only flash dump-to-CF for the Contax N Digital.
 *
 * This small routine is spliced into an unused, all-zero region of the stock 1.08 firmware and the two calls to the
 * firmware-update routine are redirected to nd_lastresort_gate(). After installation, choosing the camera's firmware
 * update command no longer updates anything:
 *
 *   - marker file A:\NDDUMP.RUN present -> copy the whole 1 MiB flash window (memory-mapped at 0xBC000000) to a NEW
 *     file A:\NDDUMP.BIN, with a 16-byte header and a 12-byte CRC trailer, then return.
 *   - marker absent (or it cannot be opened) -> return -10 and do nothing (fail-closed: the updater is never called).
 *
 * Properties:
 *   - Read-only: the flash is read as ordinary memory; no flash erase/program commands are ever issued.
 *   - No overwrite: if A:\NDDUMP.BIN can already be opened, the dump is refused (-1). Rename/move old dumps first.
 *     (The firmware's open() has no known exclusive-create flag, so this is a read-probe, not an atomic guarantee.)
 *   - Dual CRC: the flash is CRC32'd while writing and again on a second read-only pass; both values are stored.
 *   - Fail-safe: any open/write/close error stops immediately with a distinct negative code (-1..-6).
 *
 * WARNING: installing this firmware uses the camera's stock updater, which erases and reprograms flash. An interrupted
 * or failed install can permanently brick the camera. See README.md.
 */
#include "nd_rom.h"

#define ND_LR_MAGIC0 0x4E444C52u /* "NDLR" */
#define ND_LR_MAGIC1 0x44554D50u /* "DUMP" */

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

/* Read-only full-NOR dump to a NEW card file, with existing-file refusal and a dual-CRC integrity trailer. */
int nd_lastresort_backup(void)
{
    const unsigned int base = 0xBC000000u;    /* ND_FLASH_BASE as a header word */
    const unsigned int len  = ND_FLASH_SIZE;  /* 1 MiB window (true device size not yet confirmed) */
    const unsigned int CHUNK = 0x8000u;

    /* Existing-file refusal: if the output opens read-only, it already exists -> do not overwrite. */
    int ex = ND_OPEN("A:\\NDDUMP.BIN", 0x8000, 0x80);
    if (ex >= 0) { ND_CLOSE(ex); return -1; }

    int fd = ND_OPEN("A:\\NDDUMP.BIN", 0x8102, 0x180); /* create+write, NEW file */
    if (fd < 0) return -2;

    unsigned int header[4] = { ND_LR_MAGIC0, ND_LR_MAGIC1, base, len };
    if (write_all(fd, header, sizeof header)) { ND_CLOSE(fd); return -3; }

    unsigned int crc = 0, off = 0;
    while (off < len) {
        unsigned int n = len - off;
        if (n > CHUNK) n = CHUNK;
        const unsigned char *src = (const unsigned char *)(base + off); /* memory-mapped read; no CUI command */
        if (write_all(fd, src, n)) { ND_CLOSE(fd); return -4; }
        crc = crc32_update(crc, src, n);
        off += n;
    }

    /* Second integrity pass: re-read the flash to detect read instability (independent of the card). */
    unsigned int crc2 = 0;
    for (off = 0; off < len; off += CHUNK) {
        unsigned int n = len - off;
        if (n > CHUNK) n = CHUNK;
        crc2 = crc32_update(crc2, (const unsigned char *)(base + off), n);
    }

    unsigned int trailer[3] = { crc, crc2, len };     /* crc2==crc confirms a stable read */
    if (write_all(fd, trailer, sizeof trailer)) { ND_CLOSE(fd); return -5; }

    return (ND_CLOSE(fd) < 0) ? -6 : 0;
}

/* Fail-closed gate: entry spliced in front of the update worker. Marker present -> dump; otherwise do NOTHING. */
__attribute__((section(".text.entry")))
int nd_lastresort_gate(void *arg)
{
    (void)arg;
    int fd = ND_OPEN("A:\\NDDUMP.RUN", 0x8000, 0x80); /* read-only marker probe */
    if (fd >= 0) {
        ND_CLOSE(fd);
        return nd_lastresort_backup();   /* operator asked for a dump */
    }
    return -10;                          /* FAIL-CLOSED: no marker -> never update, never dump */
}
