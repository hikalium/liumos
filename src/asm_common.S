.intel_syntax noprefix

.global Die
Die:
	cli
	hlt
	jmp Die

