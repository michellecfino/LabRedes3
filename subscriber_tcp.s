# subscriber_udp.s
.section .data
    msg_prompt:     .ascii "Nombre del deportista al que quieras seguir 🐮: "
    msg_prompt_len = . - msg_prompt
    msg_waiting:    .ascii "Listooo. Esperando noticias...\n"
    msg_waiting_len = . - msg_waiting
    msg_news:       .ascii "📢 ¡NOTICIA!: "
    msg_news_len = . - msg_news
    prefix_sub:     .ascii "SUB:"
    newline:        .ascii "\n"

.section .bss
    .lcomm tema, 50
    .lcomm buffer, 1024
    .lcomm serv_addr, 16

.section .text
    .globl _start

_start:
    # 1. socket(AF_INET, SOCK_DGRAM, 0)
    movq $41, %rax
    movq $2, %rdi             # AF_INET
    movq $2, %rsi             # SOCK_DGRAM (UDP para tu Lab3)
    movq $0, %rdx
    syscall
    movq %rax, %r12           # r12 = sockfd

    # 2. Preparar serv_addr (127.0.0.1:8081)
    # 0x0100007F = 127.0.0.1 | 0x1F1F = 8081
    subq $16, %rsp
    movw $2, (%rsp)
    movw $0x1F1F, 2(%rsp)
    movl $0x0100007F, 4(%rsp)

    # 3. Pedir tema (scanf manual)
    movq $1, %rax
    movq $1, %rdi
    movq $msg_prompt, %rsi
    movq $msg_prompt_len, %rdx
    syscall

    # read(stdin, tema, 50)
    movq $0, %rax
    movq $0, %rdi
    movq $tema, %rsi
    movq $50, %rdx
    syscall
    movq %rax, %r13           # r13 = longitud leída (incluye \n)
    decq %r13                 # Quitamos el \n para el mensaje

    # 4. Armar "SUB:tema" y enviar
    # Reutilizamos el buffer para el mensaje de suscripción
    movq $buffer, %rdi
    movq $prefix_sub, %rsi
    movl $4, %ecx
    rep movsb                 # Copia "SUB:"
    
    movq $tema, %rsi
    movq %r13, %rcx
    rep movsb                 # Copia el tema

    # sendto(sockfd, buffer, 4 + r13, 0, &serv_addr, 16)
    movq $44, %rax
    movq %r12, %rdi
    movq $buffer, %rsi
    movq %r13, %rdx
    addq $4, %rdx             # "SUB:" + tema
    movq $0, %r10
    movq %rsp, %r8
    movq $16, %r9
    syscall

    # Mensaje de espera
    movq $1, %rax
    movq $1, %rdi
    movq $msg_waiting, %rsi
    movq $msg_waiting_len, %rdx
    syscall

main_loop:
    # 5. recvfrom(sockfd, buffer, 1024, 0, NULL, NULL)
    # En UDP no necesitamos reconectarnos, solo esperar paquetes
    movq $45, %rax            # sys_recvfrom
    movq %r12, %rdi
    movq $buffer, %rsi
    movq $1024, %rdx
    movq $0, %r10
    movq $0, %r8              # No nos importa el origen ahora
    movq $0, %r9
    syscall
    movq %rax, %r14           # r14 = bytes recibidos

    # Imprimir "¡NOTICIA!: "
    movq $1, %rax
    movq $1, %rdi
    movq $msg_news, %rsi
    movq $msg_news_len, %rdx
    syscall

    # Imprimir el contenido del buffer
    movq $1, %rax
    movq $1, %rdi
    movq $buffer, %rsi
    movq %r14, %rdx
    syscall

    # Imprimir un salto de línea extra
    movq $1, %rax
    movq $1, %rdi
    movq $newline, %rsi
    movq $1, %rdx
    syscall

    jmp main_loop