SECTIONS
{
. = 0xFFFFFFFF00000000;
.text ALIGN(0x1000): { *(.text) }
. += 0x1000;
.data ALIGN(0x1000): { *(.data) }
}
