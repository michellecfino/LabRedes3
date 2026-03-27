#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <tema> <mensaje>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv_addr = {AF_INET, htons(8081)};
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    struct timeval tv = {1, 0};
    // Para evitar bloques cuanso se espera el ack del broker
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char mensaje_pub[1024];
    snprintf(mensaje_pub, sizeof(mensaje_pub), "PUB:%s:%s", argv[1], argv[2]);

    int confirmado = 0;
    int intentos = 0;
    char buffer[1024];

    // Reintentar si el Broker no confirma
    while (!confirmado && intentos < 3) {
        printf("Publicando noticia (Intento %d)... ", intentos + 1);
        sendto(sock, mensaje_pub, strlen(mensaje_pub), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

        // Esperar el ACK del Broker
        int n = recvfrom(sock, buffer, 1023, 0, NULL, NULL);
        if (n < 0) {
            printf(" :c El Broker no confirmó. ");
            intentos++;
        } else {
            buffer[n] = '\0';
            //Esto es importante porque valida la respuesta del broker
            if (strstr(buffer, "ACK")) {
                printf(" ¡Publicación confirmada por el Broker!\n");
                confirmado= 1;
            }
        }
    }

    if (!confirmado) {
        printf("\n ERROR: No se pudo garantizar la publicación después de 3 intentos.\n");
        close(sock);
        return 1;
    }

    close(sock);
    return 0;
}