.section .data
    prefix: .ascii "PUB:"
    msg_log: .ascii "📡 Enviando ráfaga UDP...\n"
    msg_log_len = . - msg_log

.section .bss
    .lcomm buffer, 1024

.section .text
    .globl _start

_start:
    popq %rax                # argc
    cmpq $3, %rax
    jl exit

    popq %rbx                # argv[0]
    popq %r14                # argv[1] (tema)
    popq %r15                # argv[2] (noticia)

    # 1. Construir el mensaje "PUB:tema:noticia"
    movq $buffer, %rdi
    movq $prefix, %rsi
    movl $4, %ecx
    rep movsb

copy_t:
    movb (%r14), %al
    testb %al, %al
    jz add_sep
    movb %al, (%rdi)
    incq %rdi
    incq %r14
    jmp copy_t

add_sep:
    movb $':', (%rdi)
    incq %rdi

copy_n:
    movb (%r15), %al
    testb %al, %al
    jz setup_net
    movb %al, (%rdi)
    incq %rdi
    incq %r15
    jmp copy_n

setup_net:
    movq %rdi, %rdx
    subq $buffer, %rdx       
    movq %rdx, %r13          # Longitud total en r13

    # Socket
    movq $41, %rax
    movq $2, %rdi
    movq $2, %rsi
    movq $0, %rdx
    syscall
    movq %rax, %r12

    # Preparar dirección
    subq $16, %rsp
    movw $2, (%rsp)
    movw $0x911F, 2(%rsp)    # Puerto 8081
    movl $0x0100007F, 4(%rsp)# 127.0.0.1

    # --- BUCLE DE 10 ENVÍOS ---
    movq $10, %rbp           # Contador
envio_bucle:
    movq $44, %rax           # sys_sendto
    movq %r12, %rdi
    movq $buffer, %rsi
    movq %r13, %rdx
    movq $0, %r10
    movq %rsp, %r8
    movq $16, %r9
    syscall

    decq %rbp
    jnz envio_bucle

    # Log final
    movq $1, %rax
    movq $1, %rdi
    movq $msg_log, %rsi
    movq $msg_log_len, %rdx
    syscall

    addq $16, %rsp
exit:
    movq $60, %rax
    xorq %rdi, %rdi
    syscall