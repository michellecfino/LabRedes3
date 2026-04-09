#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char mensaje[BUF_SIZE];

    if (argc < 3) {
        fprintf(stderr, "Uso: %s <evento> <novedad>\n", argv[0]);
        return 1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear socket"); // str_err_sock
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "192.168.1.7", &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Dirección IP inválida\n");
        close(sock);
        return 1;
    }

    printf("Conectando al Broker...\n");
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Conexión fallida. Revisa si el Broker está corriendo");
        close(sock);
        return 1;
    }

    snprintf(mensaje, sizeof(mensaje), "PUB:%s:%s\n", argv[1], argv[2]);

    if (send(sock, mensaje, strlen(mensaje), 0) < 0) {
        perror("Error al enviar el mensaje");
    } else {
        printf("Mensaje enviado con éxito: %s", mensaje);
    }

    close(sock);
    return 0;
}