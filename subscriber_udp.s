.section .data
    msg_prompt:     .ascii "¿Qué tema quieres seguir? 🐮: "
    msg_prompt_len = . - msg_prompt
    msg_news:       .ascii "\n✨ [NUEVA NOTICIA]: "
    msg_news_len = . - msg_news
    prefix_sub:     .ascii "SUB:"

.section .bss
    .lcomm tema, 50
    .lcomm buffer, 1024

.section .text
    .globl _start

_start:
    # 1. Socket
    movq $41, %rax
    movq $2, %rdi
    movq $2, %rsi
    movq $0, %rdx
    syscall
    movq %rax, %r12

    # 2. Leer tema
    movq $1, %rax
    movq $1, %rdi
    movq $msg_prompt, %rsi
    movq $msg_prompt_len, %rdx
    syscall

    movq $0, %rax
    movq $0, %rdi
    movq $tema, %rsi
    movq $50, %rdx
    syscall
    movq %rax, %r13
    decq %r13                # Quitar \n

    # 3. Formatear "SUB:tema"
    movq $buffer, %rdi
    movq $prefix_sub, %rsi
    movq $4, %rcx
    rep movsb
    movq $tema, %rsi
    movq %r13, %rcx
    rep movsb

    # 4. Enviar suscripción al Broker
    subq $16, %rsp
    movw $2, (%rsp)
    movw $0x911F, 2(%rsp)    # 8081
    movl $0x0100007F, 4(%rsp)# 127.0.0.1
    
    movq $44, %rax
    movq %r12, %rdi
    movq $buffer, %rsi
    movq %r13, %rdx
    addq $4, %rdx            
    movq $0, %r10
    movq %rsp, %r8
    movq $16, %r9
    syscall
    addq $16, %rsp

main_loop:
    movq $45, %rax           # sys_recvfrom
    movq %r12, %rdi
    movq $buffer, %rsi
    movq $1024, %rdx
    movq $0, %r10
    xorq %r8, %r8
    xorq %r9, %r9
    syscall
    movq %rax, %r14          # Bytes recibidos

    movq $1, %rax
    movq $1, %rdi
    movq $msg_news, %rsi
    movq $msg_news_len, %rdx
    syscall

    movq $1, %rax
    movq $1, %rdi
    movq $buffer, %rsi
    movq %r14, %rdx
    syscall
    jmp main_loop