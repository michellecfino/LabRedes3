    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <arpa/inet.h>

    #define PORT 8080

    int main(int argc, char *argv[]) {

        //es para saber el nombre del archivo c:
        int sock = 0;
        //Aquí guardo la dirección del broker al que me voy a conectar
        struct sockaddr_in serv_addr;
        char mensaje[1024];

        if (argc < 3) {
            fprintf(stderr, "Uso: %s <evento> <novedad>\n", argv[0]);
            return 1;
        }

        //Importanteeee se crea el socket, SOCKET_STREAM para decir que es TCP y que garantice el orden u demás cosas que sólo el tcp puede garantizar
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Error al crear socket");
            return -1;
        }

        //Eso es para tomar la dirección del broker que está al revés jiji
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        //Aquí va la ip del broker y la pasa a binario
        if (inet_pton(AF_INET, "10.145.100.56", &serv_addr.sin_addr) <= 0) {
            fprintf(stderr, "Dirección IP inválida\n");
            return -1;
        }

    //pARA EL ERROR DE CONEXIÓN JII
        printf("Conectando al Broker...\n");
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Conexión fallida. Revisa si el Broker está corriendo");
            return -1;
        }

        //aquí se crea el paquete :D
        snprintf(mensaje, sizeof(mensaje), "PUB:%s:%s\n", argv[1], argv[2]);

        //aquí ocurre el envío del mensaje al broker yeeei
        if (send(sock, mensaje, strlen(mensaje), 0) < 0) {
            perror("Error al enviar el mensaje");
        } else {
            printf("Mensaje enviado con éxito: %s", mensaje);
        }

        //Se apaga todo :3
        close(sock);
        return 0;
    }