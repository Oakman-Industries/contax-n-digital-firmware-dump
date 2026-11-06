/* Copyright (c) 2026 Oakman Industries. Licensed under CC BY-NC-SA 4.0
 *
 * nd.ld — link the NDDUMP code at its fixed runtime address inside the stock image: VA 0x800A2DC8, which is file
 * offset 0xA2EB0 of the 1.08 CXBRFIRM.BIN (an unused all-zero region). The gate must be first (.text.entry).
 * Position-fixed, not PIC; no writable data allowed. */
OUTPUT_ARCH(mips)
ENTRY(nd_lastresort_gate)
CAVE_VA = DEFINED(CAVE_VA) ? CAVE_VA : 0x800A2DC8;
SECTIONS
{
  . = CAVE_VA;
  .text ALIGN(4) : { *(.text.entry) *(.text) *(.text.*) }
  .rodata : { *(.rodata) *(.rodata.*) }
  .data : { *(.data) *(.data.*) }
  .bss  : { *(.bss) *(.bss.*) *(COMMON) }
  _end = .;
  /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.comment) *(.pdr) *(.gnu.attributes) *(.note*) }
}
ASSERT(SIZEOF(.data) == 0, "NDDUMP blob must have no initialised .data")
ASSERT(SIZEOF(.bss) == 0, "NDDUMP blob must have no .bss")
