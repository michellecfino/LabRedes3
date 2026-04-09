#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PORT 8081
#define MAX_SUBS 50
#define BUFFER_SIZE 1024

typedef struct {
    struct sockaddr_in addr;
    char disciplina[50];
    int activo;
} Suscriptor;

typedef struct {
    uint32_t seq; //Aquí está la secuencia :D
    uint32_t es_ack;
    char mensaje[256];
} PaqueteQUIC;

Suscriptor subs[MAX_SUBS];

int main() {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    srand(time(NULL));

    //aquí se crea el socker c:
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

    for(int i = 0; i < MAX_SUBS; i++) subs[i].activo = 0;

    printf("Broker UDP/QUIC encendido en puerto %d\n", PORT);
    printf("Esperando mensajes... 🐮\n\n");

    while (1) {
        struct timeval no_tv = {0, 0}; 
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &no_tv, sizeof(no_tv));

        int n = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &addr_len);
        
        if (n > 0) {
            buffer[n] = '\0'; 

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
                            printf("[NUEVA SUSCRIPCIÓN] %s:%d interesado en %s\n", 
                                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), disciplina);
                            break;
                        }
                    }
                }
            } 
            else if (tipo && strcmp(tipo, "PUB") == 0 && disciplina && contenido) {
                printf("\n [NOTICIA] %s -> %s\n", disciplina, contenido);
                
                for (int i = 0; i < MAX_SUBS; i++) {
                    if (subs[i].activo && strcmp(subs[i].disciplina, disciplina) == 0) {
                        
                        PaqueteQUIC noticia;
                        noticia.seq = rand() % 1000 + 1; //Aquí se crea la secuencia
                        noticia.es_ack = 0;
                        strncpy(noticia.mensaje, contenido, 255);

                        struct timeval tv = {1, 0}; 
                        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

                        int intentos = 0;
                        int confirmado = 0;

                        //Aquí está la retransmisión

                        while (intentos < 3 && !confirmado) {
                            sendto(sock, &noticia, sizeof(noticia), 0, 
                                   (struct sockaddr *)&subs[i].addr, sizeof(subs[i].addr));
                            
                            printf("[QUIC] Enviando Seq %d a Puerto %d (Intento %d)...\n", 
                                    noticia.seq, ntohs(subs[i].addr.sin_port), intentos + 1);

                            PaqueteQUIC respuesta;
                            if (recvfrom(sock, &respuesta, sizeof(respuesta), 0, NULL, NULL) > 0) {
                                if (respuesta.es_ack == 1 && respuesta.seq == noticia.seq) {
                                    printf("[QUIC] ACK recibido para Seq %d\n", noticia.seq);
                                    confirmado = 1;
                                }
                            } else {
                                intentos++;
                                printf("[QUIC] Timeout! Reintentando...\n");
                            }
                        }

                        if (!confirmado) {
                            printf("[QUIC] No se pudo entregar a este suscriptor.\n");
                        }
                    }
                }
                printf("-----------------------------------\n");
            }
        }
    }
    return 0;
}