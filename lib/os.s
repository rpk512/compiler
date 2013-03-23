; vim: syntax=nasm

section .text
os.exit:
    mov rax, 60
    mov rdi, [rsp]
    syscall
