#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <disciplina> <novedad>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Error al crear el socket");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8081);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    char mensaje_pub[1024];
    snprintf(mensaje_pub, sizeof(mensaje_pub), "PUB:%s:%s", argv[1], argv[2]);

    printf("📡 Enviando novedad de %s al Broker...\n", argv[1]);
    
    int bytes_enviados = sendto(sock, mensaje_pub, strlen(mensaje_pub), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (bytes_enviados < 0) {
        perror("Error al enviar el datagrama");
    } else {
        printf("Mensaje de %d bytes enviado con éxito. (Quién sabe si llegó)\n", bytes_enviados);
    }

    close(sock);
    return 0;
}