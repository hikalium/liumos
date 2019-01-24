SECTIONS
{
. = 0xFFFFFFFF00000000;
.text : { *(.text) } :text
. = . + 0x1000;
.data : { *(.data) } :data
}
