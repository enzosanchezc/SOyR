#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PORT 1234

int main()
{
    int client_fd, socket_con, PID[4];
    struct sockaddr_in server;
    int addrlen = sizeof(server);
    int player_number, total_players;
    char rx_buffer[1024];
    char tx_buffer[1024];

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("[-] Error la creación del socket");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

    if (connect(client_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("[-] Error en la conexión");
        exit(EXIT_FAILURE);
    }

    // system("clear");
    puts("Conectado al servidor");
    puts("Bienvenido a 3x5!");

    recv(client_fd, rx_buffer, 1024, 0);
    if (strcmp(rx_buffer, "Sos el primer jugador en conectarse, ¿cuantos van a jugar la partida 2, 3 o 4?\n") == 0)
    {
        printf("%s", rx_buffer);
        while (1)
        {
            printf("Ingrese el número de jugadores: ");
            scanf("%s", tx_buffer);
            int players = atoi(tx_buffer);
            if (!(players == 2 || players == 3 || players == 4))
            {
                printf("Ingrese un número válido\n");
            }
            else
            {
                send(client_fd, tx_buffer, 1024, 0);
                break;
            }
        }
    }
    else
    {
        printf("%s", rx_buffer);
    }

    // El servidor pregunta al cliente su nombre
    memset(rx_buffer, 0, 1024);
    recv(client_fd, rx_buffer, 1024, 0); // Decime tu nombre:
    printf("%s", rx_buffer);
    scanf("%s", tx_buffer);
    send(client_fd, tx_buffer, 1024, 0);

    // Si es el ultimo jugador, recibe el mensaje que todos estan listos. Sino, que espera a los demas jugadores
    memset(rx_buffer, 0, 1024);
    recv(client_fd, rx_buffer, 1024, 0);
    printf("%s", rx_buffer);

    if (strcmp(rx_buffer, "Esperando a los demás jugadores...\n") == 0)
    {
        // Si no es el ultimo jugador, espera a recibir el mensaje que todos estan listos
        recv(client_fd, rx_buffer, 1024, 0);
        printf("%s", rx_buffer);
    }

    memset(rx_buffer, 0, 1024);

    // ARRANCA EL JUEGO
    int fin = 0;
    int turno = 0;
    while (fin == 0)
    {
        recv(client_fd, rx_buffer, 1024, 0);

        // separo el mensaje recibido con strtok con separador "\n"
        char *token = strtok(rx_buffer, "\n");
        // Si el ultimo token es "Levantás (L) o descartás (D)"
        while (token != NULL)
        {
            token = strtok(NULL, "\n");
        }
        if (strcmp(token, "Levantás (L) o descartás (D)") == 0)
        {
            printf("%s", rx_buffer);
            scanf("%s", tx_buffer);
            // corroborar que el jugador ingrese L o D
            while (strcmp(tx_buffer, "L") != 0 && strcmp(tx_buffer, "D") != 0)
            {
                printf("No válido, ingrese L o D\n");
                scanf("%s", tx_buffer);
            }
            turno = 1;
        }
        else if (strcmp(token, "Tenés que descartar") == 0)
        {
            printf("%s", rx_buffer);
            turno = 1;
        }
        if (turno == 1)
        {
            // Tu carta(a, b o c)
            memset(rx_buffer, 0, 1024);
            recv(client_fd, rx_buffer, 1024, 0);
            printf("%s", rx_buffer);
            scanf("%s", tx_buffer);
            // corroborar que la letra sea valida
            switch (strlen(rx_buffer))
            {
            case 14:
                while (strcmp(rx_buffer, "a") != 0)
                {
                    printf("No válido, ingrese una de las opciones indicadas\n");
                    scanf("%s", tx_buffer);
                }
                break;
            case 18:
                while (strcmp(rx_buffer, "a") != 0 && strcmp(rx_buffer, "b") != 0)
                {
                    printf("No válido, ingrese una de las opciones indicadas\n");
                    scanf("%s", tx_buffer);
                }
                break;
            case 21:
                while (strcmp(rx_buffer, "a") != 0 && strcmp(rx_buffer, "b") != 0 && strcmp(rx_buffer, "c") != 0)
                {
                    printf("No válido, ingrese una de las opciones indicadas\n");
                    scanf("%s", tx_buffer);
                }
                break;
            }
            send(client_fd, tx_buffer, 1024, 0);

            // Carta sobre la mesa (a, b, c, d)
        }
        return 0;
    }
}