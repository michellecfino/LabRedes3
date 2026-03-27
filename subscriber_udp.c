#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv_addr = {AF_INET, htons(8081)};
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr); 

    // TIEMNPO 2 SEGUNDOS
    struct timeval tv = {2, 0}; 
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char tema[50];
    printf("⛸️ ¿A qué tema seguimos (UDP)? ");
    scanf("%s", tema);

    char mensaje_sub[64], buffer[1024];
    snprintf(mensaje_sub, sizeof(mensaje_sub), "SUB:%s", tema);

    int confirmado = 0, intentos = 0;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    // RETRANSMISIÓN
    while (!confirmado && intentos < 3) {
        printf("📡 Enviando SUB (Intento %d)... ", intentos + 1);
        sendto(sock, mensaje_sub, strlen(mensaje_sub), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

        int n = recvfrom(sock, buffer, 1023, 0, (struct sockaddr *)&from_addr, &from_len);
        if (n < 0) {
            printf("Sin respuesta. Reintentando...\n");
            intentos++;
        } else {
            buffer[n] = '\0';
            // VALIDACIÓN DE ORIGEN DEL ACK
            if (from_addr.sin_addr.s_addr == serv_addr.sin_addr.s_addr && strstr(buffer, "ACK")) {
                printf(" Confirmado: %s\n", buffer);
                confirmado = 1;
            }
        }
    }

    if (!confirmado) {
        printf(" Broker inalcanzable tras 3 intentos. Saliendo.\n");
        close(sock); return 1;
    }

    // Volver a modo bloqueo infinito para recibir noticias
    tv.tv_sec = 0; 
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("🎧 Escuchando noticias de %s...\n", tema);
    while(1) {
        int n = recvfrom(sock, buffer, 1023, 0, NULL, NULL);
        if (n > 0) {
            buffer[n] = '\0';
            printf("✨ ¡NUEVO MENSAJE!: %s\n", buffer);
        }
    }
    return 0;
}