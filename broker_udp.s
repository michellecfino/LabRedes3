.section .data
    msg_start:      .ascii "🚀 Broker UDP ASM (Puerto 8081) Encendido\n"
    msg_start_len = . - msg_start
    msg_sub:        .ascii "✅ [NUEVA SUSCRIPCIÓN] Registrada\n"
    msg_sub_len = . - msg_sub
    msg_pub:        .ascii "📰 [NOTICIA] Reenviando...\n"
    msg_pub_len = . - msg_pub

.section .bss
    .lcomm buffer, 1024
    .lcomm client_addr, 16
    .lcomm addr_len, 4
    .lcomm subs_table, 800    # 50 slots * 16 bytes
    .lcomm subs_active, 50   # 50 flags (0 o 1)

.section .text
    .globl _start

_start:
    # 1. Crear Socket UDP
    movq $41, %rax           # sys_socket
    movq $2, %rdi            # AF_INET
    movq $2, %rsi            # SOCK_DGRAM
    movq $0, %rdx
    syscall
    movq %rax, %r12          # Guardar sockfd en r12

    # 2. Bind al puerto 8081
    subq $16, %rsp
    movw $2, (%rsp)          # AF_INET
    movw $0x911F, 2(%rsp)    # Puerto 8081 (Big-Endian)
    movl $0, 4(%rsp)         # INADDR_ANY (0.0.0.0)
    
    movq $49, %rax           # sys_bind
    movq %r12, %rdi
    movq %rsp, %rsi
    movq $16, %rdx
    syscall
    addq $16, %rsp           # Limpiar stack

    # Imprimir mensaje de inicio
    movq $1, %rax
    movq $1, %rdi
    movq $msg_start, %rsi
    movq $msg_start_len, %rdx
    syscall

main_loop:
    movl $16, addr_len
    movq $45, %rax           # sys_recvfrom
    movq %r12, %rdi
    movq $buffer, %rsi
    movq $1024, %rdx
    movq $0, %r10
    movq $client_addr, %r8
    movq $addr_len, %r9
    syscall
    movq %rax, %r13          # Bytes recibidos

    # Lógica: ¿SUB o PUB?
    cmpb $'S', buffer
    je handle_sub
    cmpb $'P', buffer
    je handle_pub
    jmp main_loop

handle_sub:
    xorq %rcx, %rcx
find_slot:
    cmpb $0, subs_active(%rcx)
    je found_slot
    incq %rcx
    cmpq $50, %rcx
    jl find_slot
    jmp main_loop

found_slot:
    movb $1, subs_active(%rcx)
    imulq $16, %rcx, %rax
    # Copiar sockaddr_in (16 bytes)
    movq client_addr, %rdx
    movq %rdx, subs_table(%rax)
    movq client_addr+8, %rdx
    movq %rdx, subs_table+8(%rax)

    # Log suscripción
    movq $1, %rax
    movq $1, %rdi
    movq $msg_sub, %rsi
    movq $msg_sub_len, %rdx
    syscall
    jmp main_loop

handle_pub:
    xorq %rbx, %rbx
loop_pub:
    cmpb $1, subs_active(%rbx)
    jne next_sub
    
    imulq $16, %rbx, %r11
    addq $subs_table, %r11
    
    movq $44, %rax           # sys_sendto
    movq %r12, %rdi
    movq $buffer, %rsi
    movq %r13, %rdx
    movq $0, %r10
    movq %r11, %r8           # Puntero a la dirección guardada
    movq $16, %r9
    syscall

next_sub:
    incq %rbx
    cmpq $50, %rbx
    jl loop_pub
    jmp main_loop