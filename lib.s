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

global print_line
print_line:
    push rbp
    mov rbp, rsp

    mov rax, [rbp+16]
    mov ecx, [rax]
    lea rsi, [rax+4]

    sub rsp, rcx
    sub rsp, 1

    mov rdi, rsp

.loop:
    cmp rcx, 0
    je .end
    mov al, [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    dec rcx
    jmp .loop
.end:

    mov [rdi], BYTE 10

    mov rax, 1
    mov rdi, 1
    mov rsi, rsp
    mov rdx, [rbp+16]
    mov edx, [rdx]
    inc edx
    syscall

    mov rsp, rbp
    pop rbp
    ret
