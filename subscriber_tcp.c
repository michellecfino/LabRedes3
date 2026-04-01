#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "192.168.1.7", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error de conexión");
        return 1;
    }

    char tema[50];
    char mensaje[64];

    printf("Nombre del deportista al que quieras seguir 🐮: ");
    fflush(stdout);
    scanf("%s", tema);

    tema[strcspn(tema, "\n")] = 0;

    snprintf(mensaje, sizeof(mensaje), "SUB:%s", tema);

    send(sock, mensaje, strlen(mensaje), 0);
    printf("Listooo. Esperando noticias de %s...\n", tema);

    char buffer[1024];
    while(1) {
        memset(buffer, 0, 1024);

        int n = read(sock, buffer, 1023);
        
        if (n <= 0) {
            printf("\nConexión terminada.\n");
            break;
        }
        
        if (n == 1 && buffer[0] == '\n') {
            continue;
        }

        buffer[n] = '\0';
        printf("¡NOTICIA!: %s\n", buffer);
        fflush(stdout);
    }

    close(sock);
    return 0;
}