# =============================================================================
# publisher_tcp.s — Publicador TCP
# =============================================================================
.equ PORT,          8080
.equ BUF_SIZE,      1024
.equ AF_INET,       2
.equ SOCK_STREAM,   1

.section .data
str_uso:        .asciz "Uso: %s <evento> <novedad>\n"
str_conn:       .asciz "Conectando al Broker...\n"
str_ok:         .asciz "Mensaje enviado con éxito: %s\n"
str_ip_inv:     .asciz "Dirección IP inválida\n"
str_err_sock:   .asciz "Error al crear socket\n"
str_err_conn:   .asciz "Conexión fallida. Revisa si el Broker está corriendo\n"
str_err_send:   .asciz "Error al enviar el mensaje\n"

fmt_pub:        .asciz "PUB:%s:%s\n"  # Añadir newline
broker_ip:      .asciz "192.168.1.7"

server_addr:
    .word  AF_INET
    .word  0
    .int   0
    .fill  8, 1, 0

.section .bss
mensaje:    .skip BUF_SIZE
sock_fd:    .skip 4

.section .text
.global main

main:
    pushq   %rbp
    movq    %rsp, %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    subq    $8, %rsp

    movl    %edi, %ebx
    movq    %rsi, %r12

    # Verificar argumentos
    cmpl    $3, %ebx
    jge     .Largs_ok

    movq    stderr(%rip), %rdi
    leaq    str_uso(%rip), %rsi
    movq    (%r12), %rdx
    xorl    %eax, %eax
    call    fprintf
    movl    $1, %eax
    jmp     .Lmain_ret

.Largs_ok:
    # Crear socket
    movq    $41, %rax
    movq    $2, %rdi
    movq    $1, %rsi
    xorq    %rdx, %rdx
    syscall
    movl    %eax, %r13d

    testl   %r13d, %r13d
    jns     .Lsock_ok

    leaq    str_err_sock(%rip), %rdi
    call    perror
    movl    $1, %eax
    jmp     .Lmain_ret

.Lsock_ok:
    # Configurar dirección
    movw    $PORT, %di
    call    htons
    movw    %ax, server_addr+2(%rip)

    movq    $2, %rdi
    leaq    broker_ip(%rip), %rsi
    leaq    server_addr+4(%rip), %rdx
    call    inet_pton

    testl   %eax, %eax
    jg      .Lpton_ok

    movq    stderr(%rip), %rdi
    leaq    str_ip_inv(%rip), %rsi
    xorl    %eax, %eax
    call    fprintf
    movl    $1, %eax
    jmp     .Lclose_ret

.Lpton_ok:
    # Conectar
    leaq    str_conn(%rip), %rdi
    xorl    %eax, %eax
    call    printf

    movq    $42, %rax
    movl    %r13d, %edi
    leaq    server_addr(%rip), %rsi
    movq    $16, %rdx
    syscall

    testl   %eax, %eax
    jns     .Lconn_ok

    leaq    str_err_conn(%rip), %rdi
    call    perror
    movl    $1, %eax
    jmp     .Lclose_ret

.Lconn_ok:
    # Formatear mensaje PUB
    leaq    mensaje(%rip), %rdi
    movq    $BUF_SIZE, %rsi
    leaq    fmt_pub(%rip), %rdx
    movq    8(%r12), %rcx
    movq    16(%r12), %r8
    xorl    %eax, %eax
    call    snprintf

    # Enviar mensaje
    leaq    mensaje(%rip), %rdi
    call    strlen
    movq    %rax, %rdx
    movl    %r13d, %edi
    leaq    mensaje(%rip), %rsi
    xorq    %rcx, %rcx
    call    send

    testl   %eax, %eax
    jns     .Lsend_ok

    leaq    str_err_send(%rip), %rdi
    call    perror
    jmp     .Lclose_ret

.Lsend_ok:
    leaq    str_ok(%rip), %rdi
    leaq    mensaje(%rip), %rsi
    xorl    %eax, %eax
    call    printf

.Lclose_ret:
    # Cerrar socket
    movl    %r13d, %edi
    movq    $3, %rax
    syscall
    xorl    %eax, %eax

.Lmain_ret:
    addq    $8, %rsp
    popq    %r13
    popq    %r12
    popq    %rbx
    leave
    ret