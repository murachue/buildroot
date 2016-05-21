OUTPUT_ARCH(mips)
ENTRY(start)
SECTIONS
{
 . = 0xffffffff80000400;
 .text : {
  *(.text .text.*)
 }
 .data : {
  *(.data .data.*)
 }
 /* Sections to be discarded */
 /DISCARD/ : { *(.exit.text) *(.memexit.text) *(.exit.data) *(.memexit.data) *(.memexit.rodata) *(.exitcall.exit) *(.discard) *(.discard.*) }
 /DISCARD/ : {
  /* ABI crap starts here */
  *(.MIPS.abiflags)
  *(.MIPS.options)
  *(.options)
  *(.pdr)
  *(.reginfo)
  *(.eh_frame)
 }
}
