.intel_syntax noprefix
.global write
write:
	mov rax, 1
	syscall
	ret

.global exit
exit:
	mov rax, 60
	syscall
	
