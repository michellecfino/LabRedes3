#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BROKER_PORT 8081
#define BROKER_IP "127.0.0.1"

typedef struct {
    uint32_t seq;
    uint32_t es_ack;
    char mensaje[256]; 
} PaqueteQUIC;

int main() {
    int sock;
    struct sockaddr_in serv_addr, client_addr;
    char tema[50];
    char mensaje_sub[64];
    socklen_t addr_len = sizeof(serv_addr);

    //aquí también se crea el socket c:
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear socket");
        return 1;
    }

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(0);

    if (bind(sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error en bind local");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(BROKER_PORT);
    inet_pton(AF_INET, BROKER_IP, &serv_addr.sin_addr);

    printf("¿Qué quieres seguir? 🐮: ");
    scanf("%s", tema);

    snprintf(mensaje_sub, sizeof(mensaje_sub), "SUB:%s", tema);
    sendto(sock, mensaje_sub, strlen(mensaje_sub), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    printf("⌛ Esperando novedades de %s... (Ctrl+C para salir)\n", tema);

    while (1) {
        PaqueteQUIC recibo;
        int n = recvfrom(sock, &recibo, sizeof(recibo), 0, (struct sockaddr *)&serv_addr, &addr_len);

        if (n > 0 && recibo.es_ack == 0) {
            printf("\n✨ [NUEVA NOTICIA Seq %d]: %s\n", recibo.seq, recibo.mensaje);

            PaqueteQUIC confirmacion;
            confirmacion.seq = recibo.seq;
            confirmacion.es_ack = 1;
            strcpy(confirmacion.mensaje, "ACK");

            sendto(sock, &confirmacion, sizeof(confirmacion), 0, (struct sockaddr *)&serv_addr, addr_len);
            printf("📡 [QUIC] ACK enviado para Seq %d\n", recibo.seq);
            printf("Esperando más... 🐮\n");
        }
    }

    close(sock);
    return 0;
}