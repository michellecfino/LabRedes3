# Sistema Pub/Sub con Broker (TCP/UDP)

Proyecto de laboratorio para implementar un patrón productor-consumidor (publisher/subscriber) con un broker intermedio. Incluye versiones por socket TCP y UDP.

## Estructura

- `broker_tcp.c` - Broker TCP. Escucha suscriptores y publicaciones en puerto `8080`.
- `broker_udp.c` - Broker UDP. Escucha suscriptores y publicaciones en puerto `8081`.
- `publisher_tcp.c` - Publicador TCP.
- `subscriber_tcp.c` - Suscriptor TCP.
- `publisher_udp.c` - Publicador UDP.
- `subscriber_udp.c` - Suscriptor UDP.

También hay archivos `.s` con versiones en ensamblador del mismo programa, típicamente para comparación o ejercicios de CPU.

## Funcionamiento

### Broker UDP

- Espera mensajes UDP de clientes.
- Para `SUB:<tema>` agrega al suscriptor a la lista.
- Para `PUB:<tema>:<mensaje>` reenruta el contenido a todos los suscriptores registrados de ese tema.

### Broker TCP

- Acepta conexiones entrantes en socket TCP.
- `SUB:<tema>` deja la conexión abierta y encola el suscriptor.
- `PUB:<tema>:<mensaje>` envía la noticia a todas las conexiones suscritas al tema.

## ¿Por qué UDP y TCP?

### UDP (User Datagram Protocol)

- Protocolo sin conexión (connectionless): no se establece una sesión antes de enviar datos.
- Menor overhead y latencia, ideal para mensajes cortos y velocidad.
- No garantiza entrega, orden ni protección contra duplicados.
- En este proyecto: el broker UDP recibe y reenvía sin confirmar, por eso es muy rápido pero puede perder mensajes.

### TCP (Transmission Control Protocol)

- Protocolo con conexión (connection-oriented): establece una sesión entre cliente y servidor.
- Garantiza entrega, orden correcto y control de flujo.
- Más confiable, pero con mayor overhead y posibles retrasos.
- En este proyecto: el broker TCP mantiene conexiones activas de suscriptores y entrega las noticias de forma segura.

### Diferencias clave

- UDP es "looser" y rápido; TCP es "más estricto" y confiable.
- UDP permite que muchos clientes envíen/reciban rápido sin handshake.
- TCP mantiene estado por cliente y evita pérdidas (síntomas: reintentos, confirmaciones, control de errores).
- El flujo de datos en este laboratorio: usar UDP cuando quieres rendimiento y cierto riesgo; usar TCP cuando quieres fiabilidad.

## Compilación

Desde la carpeta del proyecto:

```bash
gcc -o bro_tcp broker_tcp.c -lpthread
gcc -o pub_tcp publisher_tcp.c
gcc -o sub_tcp subscriber_tcp.c

gcc -o bro_udp broker_udp.c
gcc -o pub_udp publisher_udp.c
gcc -o sub_udp subscriber_udp.c
```

> Nota: Para `publisher_tcp` y `subscriber_tcp` la IP fija en el código es `192.168.1.7`. Cambia esa IP por la ip del computador en el que se encuentre el broker

## Uso

### Ejemplo UDP

1. Iniciar broker UDP:
   - `./bro_udp`
2. Iniciar un suscriptor:
   - `./sub_udp`
   - Ingresar tema, por ejemplo: `Fútbol`
3. Enviar mensajes desde publisher:
   - `./pub_udp Fútbol "Gol número 1"`
   - `./pub_udp Fútbol "Gol número 2"`

El suscriptor recibirá los mensajes del tema suscrito.

### Ejemplo TCP

1. Iniciar broker TCP:
   - `./bro_tcp`
2. Iniciar un suscriptor TCP (puede ser en otra terminal):
   - `./sub_tcp`
   - Ingresar tema, por ejemplo: `Messi`
3. Enviar una publicación:
   - `./pub_tcp Messi "¡Anotó un gol!"`

El broker reenviará el texto a los suscriptores conectados al tema.

## Consideraciones

- Asegúrate de tener puertos libres (8080 y 8081).
- El broker TCP usa `SO_REUSEADDR` para reinicios rápidos.
- El broker UDP maneja hasta 50 suscriptores, TCP hasta 20.
- El sistema no implementa persistencia, confirmaciones o reintentos.

## Posibles mejoras

- Soporte de múltiples temas dinámicos (busca en arrays por hash o lista enlazada).
- Autenticación simple o control de acceso.
- Interfaz CLI con parámetros de host/puerto.
- Manejo de señales para cierre limpio (`SIGINT`) con liberación de recursos.
