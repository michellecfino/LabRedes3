#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8081
#define MAX_SUBS 50
#define BUFFER_SIZE 1024

typedef struct {
    struct sockaddr_in addr;
    char disciplina[50];
    int activo;
} Suscriptor;

Suscriptor subs[MAX_SUBS];

int main() {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        exit(1);
    }

    printf("🚀 Broker UDP encendido en puerto %d\n", PORT);
    printf("Esperando mensajes... 🐮\n\n");

    while (1) {
        int n = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &addr_len);
        
        if (n > 0) {
            buffer[n] = '\0'; 

            printf("¡Paquete recibido!");
            printf(" Origen: %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            char temp_buffer[BUFFER_SIZE];
            strcpy(temp_buffer, buffer);

            char *tipo = strtok(temp_buffer, ":");
            char *disciplina = strtok(NULL, ":");
            char *contenido = strtok(NULL, "\n");

            if (tipo && strcmp(tipo, "SUB") == 0 && disciplina) {
                int ya_existe = 0;
                for (int i = 0; i < MAX_SUBS; i++) {
                    if (subs[i].activo && 
                        subs[i].addr.sin_port == client_addr.sin_port &&
                        subs[i].addr.sin_addr.s_addr == client_addr.sin_addr.s_addr &&
                        strcmp(subs[i].disciplina, disciplina) == 0) {
                        ya_existe = 1;
                        break;
                    }
                }

                if (!ya_existe) {
                    for (int i = 0; i < MAX_SUBS; i++) {
                        if (!subs[i].activo) {
                            subs[i].addr = client_addr;
                            strncpy(subs[i].disciplina, disciplina, 49);
                            subs[i].activo = 1;
                            printf("[NUEVA SUSCRIPCIÓN] Tema: %s\n", disciplina);
                            break;
                        }
                    }
                }
            } 
            else if (tipo && strcmp(tipo, "PUB") == 0 && disciplina && contenido) {
                printf("📰 [NOTICIA] %s -> %s\n", disciplina, contenido);
                
                for (int i = 0; i < MAX_SUBS; i++) {
                    if (subs[i].activo && strcmp(subs[i].disciplina, disciplina) == 0) {
                        sendto(sock, contenido, strlen(contenido), 0, 
                               (struct sockaddr *)&subs[i].addr, sizeof(subs[i].addr));
                    }
                }
            }
            printf("-----------------------------------\n");
        }
    }
    return 0;
}