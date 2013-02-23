; vim: set syntax=nasm:

bits 64

section .text

global _start
_start:
    call main

    mov rax, 60
    mov rdi, 0
    syscall


global print64
print64:
    mov rax, [rsp+8]
    sub rsp, 32
    lea rsi, [rsp+31]
    mov [rsi], BYTE 0xA
    mov rcx, 1

.loop:
    mov rdx, 0
    mov rbx, 10
    div rbx
    add dl, '0'
    dec rsi
    mov [rsi], dl
    inc rcx
    cmp rax, 0
    jne .loop

    mov rax, 1
    mov rdi, 1
    mov rdx, rcx
    syscall

    add rsp, 32
    ret
