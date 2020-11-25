SECTIONS
{
. = 0xFFFFFFFF00000000;
.text ALIGN(0x1000): {
  *(.text*)
}
. += 0x10000;
.data ALIGN(0x1000): {
  *(.data*)
  *(.rodata*)
  *(.eh_frame_hdr)
  *(.eh_frame)
}
.got : {
  *(.got)
}
}
