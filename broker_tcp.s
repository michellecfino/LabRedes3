# =============================================================================
# Desarrollo del broker con TCP (es decir, me importa mucho saber que lo que envío, llega y está en orden)
# =============================================================================

.equ MAX_SUBS,       20
.equ SUB_SIZE,       64
.equ BACKLOG,        10

# Syscalls x86_64
.equ SYS_READ,       0
.equ SYS_WRITE,      1
.equ SYS_CLOSE,      3
.equ SYS_SOCKET,     41
.equ SYS_ACCEPT,     43
.equ SYS_BIND,       49
.equ SYS_LISTEN,     50
.equ AF_INET,        2
.equ SOCK_STREAM,    1
.equ SOL_SOCKET,     1
.equ SO_REUSEADDR,   2

.section .data
server_addr:
    .word AF_INET
    .word 0x901f         # Esto lo pongo porque toca trabajarlo en big endian
    .int 0
    .fill 8, 1, 0

subs:           .fill (MAX_SUBS * SUB_SIZE), 1, 0 
lock:           .fill 40, 1, 0

fmt_on:         .asciz "Broker encendido en puerto 8080...\n"
fmt_new_sub:    .asciz "[BROKER] Nuevo suscriptor en partido: %s\n"
fmt_pub:        .asciz "[BROKER] Publicación recibida para %s: %s\n"
delim:          .asciz ":"
str_sub:        .asciz "SUB"
str_pub:        .asciz "PUB"

.section .bss
.align 8
server_fd:      .skip 8
thread_id:      .skip 8

.section .text
.global main
.extern printf, malloc, free, strncpy, strcmp, strtok, strlen, send
.extern pthread_create, pthread_detach, pthread_mutex_init, pthread_mutex_lock, pthread_mutex_unlock, fflush, stdout

# =============================================================================
# Aquí se maneja todo lo relacionado con el sub
# =============================================================================
manejar_cliente:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $1104, %rsp

    pushq   %r12
    pushq   %r13                # Esto es importasnte porque es el contenido del socket
    pushq   %r14                # me dice si es sub o pub
    pushq   %r15                # aquí se guarda el evento

    # 1. recupera el socker
    movq    %rdi, %r13          
    movl    (%r13), %r12d       
    movq    %r13, %rdi
    call    free

    # 2. recibe y lee el mensaje
    movl    %r12d, %edi
    leaq    -1024(%rbp), %rsi   
    movq    $1024, %rdx
    xorq    %rax, %rax
    syscall
    movb    $0, (%rsi, %rax)
    cmpq    $0, %rax
    jle     .Lclose_exit
    movb    $0, -1024(%rbp, %rax)


    # 3. lo parsea porque si no no se puede ver el mensaje
    leaq    -1024(%rbp), %rdi   
    leaq    delim(%rip), %rsi
    call    strtok
    movq    %rax, %r14

    xorq    %rdi, %rdi
    leaq    delim(%rip), %rsi
    call    strtok
    movq    %rax, %r15

    testq   %r14, %r14
    jz      .Lclose_exit
    testq   %r15, %r15
    jz      .Lclose_exit

    # esta parte me asegura que se muestre bonito en consola sin espacios de más
    movq    %r15, %rdi
    call    strlen
    testq   %rax, %rax
    jz      .Lskip_clean
    decq    %rax
    leaq    (%r15, %rax), %rcx
    cmpb    $10, (%rcx)
    jne     .Lskip_clean
    movb    $0, (%rcx)
.Lskip_clean:

    # 4. ocurre la birfucación
    movq    %r14, %rdi
    leaq    str_sub(%rip), %rsi
    call    strcmp
    testl   %eax, %eax
    jz      .Llogica_sub

    movq    %r14, %rdi
    leaq    str_pub(%rip), %rsi
    call    strcmp
    testl   %eax, %eax
    jz      .Llogica_pub
    jmp     .Lclose_exit

# esta parte es importante porque es cuando también se le muestra al sub el mensaje que envió el pub
.Llogica_sub:
    leaq    lock(%rip), %rdi
    call    pthread_mutex_lock
    xorq    %rbx, %rbx
.Lfind_slot:
    leaq    subs(%rip), %rdx
    imulq   $SUB_SIZE, %rbx, %rax
    addq    %rax, %rdx
    cmpl    $0, 54(%rdx)
    jne     .Lnext_slot

    movl    %r12d, 0(%rdx)
    movl    $1, 54(%rdx)
    movq    %rdx, %r13
    leaq    4(%rdx), %rdi
    movq    %r15, %rsi
    movq    $49, %rdx
    call    strncpy

    leaq    fmt_new_sub(%rip), %rdi
    movq    %r15, %rsi
    xorl    %eax, %eax
    call    printf
    movq    stdout(%rip), %rdi
    call    fflush

    leaq    lock(%rip), %rdi
    call    pthread_mutex_unlock

.Lwait_sub:
    movl    0(%r13), %edi
    subq    $16, %rsp
    movq    %rsp, %rsi
    movq    $1, %rdx
    xorq    %rax, %rax
    syscall #se queda esperando a que el sub muera
    addq    $16, %rsp
    cmpq    $0, %rax
    jg      .Lwait_sub

    leaq    lock(%rip), %rdi
    call    pthread_mutex_lock
    movl    $0, 54(%r13)
    leaq    lock(%rip), %rdi
    call    pthread_mutex_unlock
    jmp     .Lclose_exit

.Lnext_slot:
    incq    %rbx
    cmpq    $MAX_SUBS, %rbx
    jl      .Lfind_slot
    leaq    lock(%rip), %rdi
    call    pthread_mutex_unlock
    jmp     .Lclose_exit

# aquí se maneja la información del pub
.Llogica_pub:
    xorq    %rdi, %rdi 
    leaq    delim(%rip), %rsi
    call    strtok
    movq    %rax, %r13

    testq   %r13, %r13
    jz      .Lclose_exit

    # pongo esto aquí para hacer una copia de la noticia
    leaq    -512(%rbp), %rdi
    movq    %r13, %rsi
    call    strcpy
    leaq    -512(%rbp), %r13    # modo tieso

    # aquí quito el \n porque de lo contrario me sale duplicado el "NOTICIA" en el sub y no se ve bien
    movq    %r13, %rdi
    call    strlen
    testq   %rax, %rax
    jz      .Lnoticia_limpia
    decq    %rax
    leaq    (%r13, %rax), %rcx
    cmpb    $10, (%rcx)
    jne     .Lnoticia_limpia
    movb    $0, (%rcx)
.Lnoticia_limpia:

    # Esto es para mostrar en el broker que se están recibiendo cositas también los subs
    leaq    fmt_pub(%rip), %rdi
    movq    %r15, %rsi          # aquí el evento
    movq    %r13, %rdx          # aquí la noticia
    xorl    %eax, %eax
    call    printf
    movq    stdout(%rip), %rdi
    call    fflush

    leaq    lock(%rip), %rdi
    call    pthread_mutex_lock
    
    xorq    %rbx, %rbx
.Lloop_pub:
    leaq    subs(%rip), %r14
    imulq   $SUB_SIZE, %rbx, %rax
    addq    %rax, %r14
    cmpl    $1, 54(%r14)
    jne     .Lskip_pub

    # esto lo pongo para revisar que cada dato coincida
    leaq    4(%r14), %rdi
    movq    %r15, %rsi
    call    strcmp
    testl   %eax, %eax
    jnz     .Lskip_pub

# aquí ocurre el envío de la notica

    movq    %r13, %rdi          # rdi = puntero a la noticia para strlen
    call    strlen              # rax = longitud
    movq    %rax, %rdx          # rdx = 3er arg de send (longitud)

    # Ahora sí preparamos los otros registros para enviar
    movl    0(%r14), %edi       # edi = 1er arg de send (Socket FD real)
    movq    %r13, %rsi          # rsi = 2do arg de send (Puntero al buffer)
    xorq    %rcx, %rcx          # rcx = 4to arg (Flags = 0)
    call    send                # ¡Ahora sí llega!

    #toca enviar el \n porque chistoso ensamblador
    movl    0(%r14), %edi       # Recuperar FD del suscriptor
    subq    $16, %rsp
    movb    $10, (%rsp)         # Ponemos el \n en el stack
    movq    %rsp, %rsi          # rsi apunta al \n
    movq    $1, %rdx            # rdx = 1 byte
    xorq    %rcx, %rcx
    call    send
    addq    $16, %rsp

.Lskip_pub:
    incq    %rbx
    cmpq    $MAX_SUBS, %rbx
    jl      .Lloop_pub
    
    leaq    lock(%rip), %rdi
    call    pthread_mutex_unlock
    jmp     .Lclose_exit
.Lclose_exit:
    movl    %r12d, %edi
    movq    $SYS_CLOSE, %rax
    syscall
    popq    %r15; popq %r14; popq %r13; popq %r12
    leave
    ret

# =============================================================================
# Main
# =============================================================================
main:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $16, %rsp

    leaq    lock(%rip), %rdi
    xorq    %rsi, %rsi
    call    pthread_mutex_init

    movq    $SYS_SOCKET, %rax
    movq    $AF_INET, %rdi
    movq    $SOCK_STREAM, %rsi
    xorl    %edx, %edx
    syscall
    movl    %eax, server_fd(%rip)

    movl    $1, -4(%rbp)
    movl    server_fd(%rip), %edi
    movl    $SOL_SOCKET, %esi
    movl    $SO_REUSEADDR, %edx
    leaq    -4(%rbp), %rcx
    movl    $4, %r8d
    movl    $54, %eax
    syscall

    movl    server_fd(%rip), %edi
    leaq    server_addr(%rip), %rsi
    movl    $16, %edx
    movl    $SYS_BIND, %eax
    syscall

    movl    server_fd(%rip), %edi
    movl    $BACKLOG, %esi
    movl    $SYS_LISTEN, %eax
    syscall

    leaq    fmt_on(%rip), %rdi
    xorl    %eax, %eax
    call    printf
    movq    stdout(%rip), %rdi
    call    fflush

.Laccept_loop:
    movl    server_fd(%rip), %edi
    xorq    %rsi, %rsi
    xorq    %rdx, %rdx
    movl    $SYS_ACCEPT, %eax
    syscall
    cmpl    $0, %eax
    jl      .Laccept_loop
    
    movl    %eax, %r12d
    movq    $8, %rdi
    call    malloc
    movl    %r12d, (%rax)
    
    leaq    thread_id(%rip), %rdi
    xorq    %rsi, %rsi
    leaq    manejar_cliente(%rip), %rdx
    movq    %rax, %rcx
    call    pthread_create
    movq    thread_id(%rip), %rdi
    call    pthread_detach
    jmp     .Laccept_loop