#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8080)};
    inet_pton(AF_INET, "10.145.100.97", &addr.sin_addr);

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    char tema[50];
    char mensaje[64];

    printf("Nombre del deportista al que quieras seguir 🐮 :");
    scanf("%s", tema);

    snprintf(mensaje, sizeof(mensaje), "SUB:%s", tema);

    send(sock, mensaje, strlen(mensaje), 0);
    printf("Listooo. Esperando noticias de %s...\n", tema);


    char buffer[1024];
    while(1) {
        int n = read(sock, buffer, 1024);
        if (n > 0) {
            buffer[n] = '\0';
            printf("¡NOTICIA!: %s\n", buffer);
        }
    }
    return 0;
}