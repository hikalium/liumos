SECTIONS
{
. = 0xFFFFFFFF00000000;
.text ALIGN(0x1000): {
  *(.text)
}
.rodata : {
  *(.rodata)
}
.data ALIGN(0x1000): {
  *(.data)
}
.got : {
  *(.got)
}
}
