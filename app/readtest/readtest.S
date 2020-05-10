# Copyright (c) 2020 hikalium, d0iasm

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

.intel_syntax noprefix
.global main

main:
	mov rax, 0 # read
	mov rdi, 0 # stdin
	lea rsi, [rip+buf] # buf
	mov rdx, 7 # buflen
	syscall

    mov rax, 1 # write
    mov rdi, 1
    lea rsi, [rip+buf]
    mov rdx, 7
    syscall

    mov rcx, 0x1
loop:
    # fizz
    # カウンタレジスタrcxを3で割る
    mov rdx, 0
    mov rax, rcx
    mov r15, 3
    div r15
    # 余りが0以外（= 3の倍数ではない）のときにskipfizzにジャンプする
    cmp rdx, 0
    jne skipfizz

    # fizzを出力する
    mov rdx, 0x4
    lea rsi, [fizz]
    mov rdi, 0x1
    mov rax, 0x1
    push rcx
    syscall
    pop rcx
skipfizz:
    # buzz
    # カウンタレジスタrcxを5で割る
    mov rdx, 0
    mov rax, rcx
    mov r15, 5
    div r15
    # 余りが0以外（= 5の倍数ではない）のときにskipbuzzにジャンプする
    cmp rdx, 0
    jne skipbuzz

    # buzzを出力する
    mov rdx, 0x4
    lea rsi, [buzz]
    mov rdi, 0x1
    mov rax, 0x1
    push rcx
    syscall
    pop rcx
skipbuzz:
    inc rcx
    # 改行を出力する
    mov rdx, 0x1
    lea rsi, [newline]
    mov rdi, 0x1
    mov rax, 0x1
    push rcx
    syscall
    pop rcx
    # カウンタレジスタrcxが0x10と等しくなったとき、exitにジャンプする
    cmp rcx, 0x10
    jz exit
    jmp loop
exit:
    # exitシステムコールを呼ぶ
    mov rax, 0x3c
    syscall
    
fizz:
    .ascii "fizz"
buzz:
    .ascii "buzz"
newline:
    .ascii "\n"

.data
buf:
	.byte 0,0,0,0,0,0,0,0
