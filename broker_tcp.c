#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_SUBS 20

typedef struct {
    int socket;
    char partido[50];
    int activo; 
} Suscriptor;

Suscriptor subs[MAX_SUBS];
pthread_mutex_t lock;

void *manejar_cliente(void *arg) {
    int sock = *(int*)arg;
    free(arg);
    char buffer[1024];

    int n = read(sock, buffer, 1023);
    if (n <= 0) { close(sock); return NULL; }
    buffer[n] = '\0';

    // 1. Tokenización (como en el Assembly con strtok)
    char *tipo = strtok(buffer, ":");
    char *partido = strtok(NULL, ":");
    char *contenido_raw = strtok(NULL, ""); // Tomamos el resto del mensaje

    if (tipo && strcmp(tipo, "SUB") == 0 && partido) {
        // Limpiamos el \n del partido si viene del scan
        partido[strcspn(partido, "\n")] = 0;

        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_SUBS; i++) {
            if (!subs[i].activo) {
                subs[i].socket = sock;
                strncpy(subs[i].partido, partido, 49);
                subs[i].activo = 1;
                
                printf("[BROKER] Nuevo suscriptor en partido: %s\n", partido);
                pthread_mutex_unlock(&lock);
                
                // Se queda bloqueado esperando a que el sub cierre (Lwait_sub)
                char check;
                while(read(sock, &check, 1) > 0); 
                
                pthread_mutex_lock(&lock);
                subs[i].activo = 0;
                pthread_mutex_unlock(&lock);
                printf("[BROKER] Suscriptor desconectado de: %s\n", partido);
                close(sock);
                return NULL;
            }
        }
        pthread_mutex_unlock(&lock);
    } 
    else if (tipo && strcmp(tipo, "PUB") == 0 && partido && contenido_raw) {
        // MODO TIESO: Copiamos la noticia a un buffer propio
        char noticia[512];
        strncpy(noticia, contenido_raw, 511);
        noticia[strcspn(noticia, "\n")] = 0; // Quitamos el \n duplicado

        printf("[BROKER] Publicación recibida para %s: %s\n", partido, noticia);

        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_SUBS; i++) {
            if (subs[i].activo && strcmp(subs[i].partido, partido) == 0) {
                // Enviamos la noticia
                send(subs[i].socket, noticia, strlen(noticia), 0);
                // Enviamos el \n (el "chistoso ensamblador" que necesita el sub)
                send(subs[i].socket, "\n", 1, 0);
            }
        }
        pthread_mutex_unlock(&lock);
        close(sock);
    }
    return NULL;
}

int main() {
    int server_fd, *new_sock;
    struct sockaddr_in address;
    int opt = 1;

    pthread_mutex_init(&lock, NULL);
    for(int i=0; i<MAX_SUBS; i++) subs[i].activo = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // SO_REUSEADDR para que no te dé error el puerto al reiniciar rápido
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); // El famoso 0x901f (8080)

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 10);
    printf("Broker encendido en puerto 8080...\n");

    while(1) {
        new_sock = malloc(sizeof(int));
        int client_sock = accept(server_fd, NULL, NULL);
        if (client_sock >= 0) {
            *new_sock = client_sock;
            pthread_t tid;
            pthread_create(&tid, NULL, manejar_cliente, new_sock);
            pthread_detach(tid);
        } else {
            free(new_sock);
        }
    }
    return 0;
}