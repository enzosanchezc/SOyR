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
#include <sys/wait.h>
#include <sys/sem.h>
#include <time.h>
#include <signal.h>

#include "functions.h"

#define sem_wait(SEM_NUM)        \
    operacion.sem_num = SEM_NUM; \
    operacion.sem_op = -1;       \
    semop(semid, &operacion, 1);
#define sem_post(SEM_NUM)        \
    operacion.sem_num = SEM_NUM; \
    operacion.sem_op = 1;        \
    semop(semid, &operacion, 1);
#define SEM_KEY 0x4568
#define SHM_KEY1 0x1234
#define SHM_KEY2 0x5678
#define SHM_KEY3 0x5679
#define SHM_KEY4 0x1235
#define SHM_KEY5 0x1236
#define SHM_KEY6 0x1237
#define PORT 1234

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

typedef struct jugador
{
    int id;
    char nombre[32];
    int escobas;
    int puntos;
    int mazo[40];
    int mano[40];
    int ultimo_en_levantar;
} jugador;

int main()
{
    int server_fd, socket_con, PID[4];
    struct sockaddr_in server;
    int addrlen = sizeof(server);
    int player_number, total_players;
    char rx_buffer[1024] = {0};
    char tx_buffer[1024] = {0};

    key_t shmkey1 = ftok("/bin/ls", SHM_KEY1);
    int shmid1 = shmget(shmkey1, 4 * sizeof(jugador), 0666 | IPC_CREAT);
    jugador *jugadores = shmat(shmid1, NULL, 0);

    key_t shmkey2 = ftok("/bin/ls", SHM_KEY2);
    int shmid2 = shmget(shmkey2, 40 * sizeof(int), 0666 | IPC_CREAT);
    int *mazo = shmat(shmid2, NULL, 0);

    key_t shmkey3 = ftok("/bin/ls", SHM_KEY3);
    int shmid3 = shmget(shmkey3, 40 * sizeof(int), 0666 | IPC_CREAT);
    int *mesa = shmat(shmid3, NULL, 0);

    key_t shmkey4 = ftok("/bin/ls", SHM_KEY4);
    int shmid4 = shmget(shmkey4, sizeof(int), 0666 | IPC_CREAT);
    int *fin = shmat(shmid4, NULL, 0);

    key_t shmkey5 = ftok("/bin/ls", SHM_KEY5);
    int shmid5 = shmget(shmkey5, 40 * sizeof(int), 0666 | IPC_CREAT);
    int *mazo_temp = shmat(shmid5, NULL, 0);

    key_t shmkey6 = ftok("/bin/ls", SHM_KEY6);
    int shmid6 = shmget(shmkey6, sizeof(int), 0666 | IPC_CREAT);
    int *carta_jugada = shmat(shmid6, NULL, 0);

    key_t semkey = ftok("/bin/ls", SEM_KEY);
    union semun arg;
    struct sembuf operacion;
    operacion.sem_flg = 0;

    int semid = semget(semkey, 3, IPC_CREAT | 0666);

    arg.val = 0;
    semctl(semid, 0, SETVAL, arg);
    arg.val = 1;
    semctl(semid, 1, SETVAL, arg);
    arg.val = 0;
    semctl(semid, 2, SETVAL, arg);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("\033[0;31m[-]\033[0m Falló la creación del socket");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("\033[0;31m[-]\033[0m Falló el bind");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 4);

    for (int i = 0; i < 40; i++)
    {
        mazo[i] = 1;
        mesa[i] = 0;
    }
    repartir_mesa(mazo, mesa);
    repartir_mesa(mazo, mesa);
    repartir_mesa(mazo, mesa);
    repartir_mesa(mazo, mesa);
    printf("[*] Mesa: ");
    mano_a_string(mesa, tx_buffer);
    printf("%s", tx_buffer);
    player_number = 0;
    while (1)
    {
        if ((socket_con = accept(server_fd, (struct sockaddr *)&server, (socklen_t *)&addrlen)) < 0)
        {
            perror("\033[0;31m[-]\033[0m No se pudo establecer la conexión");
            exit(EXIT_FAILURE);
        }
        printf("\033[0;32m[+]\033[0m Conexión establecida desde %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));

        if (player_number == 0)
        {
            send(socket_con, "Sos el primer jugador en conectarse, ¿cuantos van a jugar la partida 2, 3 o 4?\nIngrese el número de jugadores: ", 113, 0);
            recv(socket_con, rx_buffer, 1024, 0);
            int players = atoi(rx_buffer);
            while (!(players == 2 || players == 3 || players == 4))
            {
                send(socket_con, "Ingrese un número válido: ", 29, 0);
                memset(rx_buffer, 0, 1024);
                recv(socket_con, rx_buffer, 1024, 0);
                players = atoi(rx_buffer);
            }
            total_players = atoi(rx_buffer);
            printf("[*] Total de jugadores: %d\n", total_players);
        }
        else
        {
            sprintf(tx_buffer, "Sos el jugador %d de un total de %d\n", player_number + 1, total_players);
            send(socket_con, tx_buffer, strlen(tx_buffer), 0);
        }

        jugadores[player_number].id = player_number;
        jugadores[player_number].escobas = 0;
        jugadores[player_number].puntos = 0;
        for (int i = 0; i < 40; i++)
        {
            jugadores[player_number].mazo[i] = 0;
            jugadores[player_number].mano[i] = 0;
        }
        jugadores[player_number].ultimo_en_levantar = 0;

        PID[player_number] = fork();
        if (PID[player_number] == 0)
        {
            close(server_fd);
            send(socket_con, "Decime tu nombre: ", 18, 0);
            recv(socket_con, rx_buffer, 1024, 0);
            while (rx_buffer[0] == '\n')
            {
                send(socket_con, "Ingrese un nombre válido: ", 29, 0);
                recv(socket_con, rx_buffer, 1024, 0);
            }
            rx_buffer[strlen(rx_buffer) - 1] = '\0';
            strcpy(jugadores[player_number].nombre, rx_buffer);
            printf("[*] El jugador número %d eligió el nombre %s\n", player_number + 1, jugadores[player_number].nombre);

            if (player_number < total_players - 1)
            {
                send(socket_con, "Esperando a los demás jugadores...\n", 36, 0);
                sem_wait(0);
            }
            else
            {
                for (int i = 0; i < total_players - 1; i++)
                {
                    sem_post(0);
                }
            }

            char *nombres_string = malloc(100);
            strcpy(nombres_string, jugadores[0].nombre);
            for (int i = 1; i < total_players - 1; i++)
            {
                strcat(nombres_string, ", ");
                strcat(nombres_string, jugadores[i].nombre);
            }
            strcat(nombres_string, " y ");
            strcat(nombres_string, jugadores[total_players - 1].nombre);

            sprintf(tx_buffer, "Ya están todos los jugadores, %s. Inicia el juego %s\n", nombres_string, jugadores[0].nombre);
            send(socket_con, tx_buffer, strlen(tx_buffer), 0);
            free(nombres_string);

            // ARRANCA EL JUEGO
            int turno = 0;
            *fin = 0;
            int vuelta = 0;
            int suma = 0;
            int cartas_en_mazo_restantes;
            int mano_temp[40], mesa_temp[40];

            while (*fin == 0)
            {
                sem_wait(1);
                repartir3(mazo, jugadores[player_number].mano);
                printf("[*] Mano del jugador %d: ", player_number + 1);
                mano_a_string(jugadores[player_number].mano, tx_buffer);
                printf("%s", tx_buffer);
                // imprimi cuantas cartas quedan en el mazo
                printf("[*] Cartas en el mazo: %d\n", contar_cartas(mazo));
                sem_post(1);

                while (vuelta < 3)
                {
                    while (turno < total_players)
                    {
                        if (player_number == turno)
                        {
                            // ACA JUEGA EL JUGADOR DE TURNO
                            // La variable restart se utiliza para que el jugador pueda volver a jugar si arma una jugada no valida
                            int restart = 0;
                            for (int i = 0; i < 40; i++)
                            {
                                mazo_temp[i] = 0;
                            }
                            // Levantás (L) o descartás (D)
                            if (contar_cartas(mesa) != 0)
                            {
                                sprintf(tx_buffer, "Tus cartas son:\n");
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                mano_a_string(jugadores[player_number].mano, tx_buffer);
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                sprintf(tx_buffer, "Las cartas sobre la mesa son:\n");
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                mano_a_string(mesa, tx_buffer);
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                sprintf(tx_buffer, "Es tu turno, %s\n", jugadores[player_number].nombre);
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                sprintf(tx_buffer, "Levantás (L) o descartás (D): ");
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                recv(socket_con, rx_buffer, 1024, 0);
                                while (rx_buffer[0] != 'L' && rx_buffer[0] != 'D')
                                {
                                    send(socket_con, "Ingrese una opción válida: ", 29, 0);
                                    recv(socket_con, rx_buffer, 1024, 0);
                                }
                            }
                            else
                            {
                                sprintf(tx_buffer, "Tus cartas son:\n");
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                mano_a_string(jugadores[player_number].mano, tx_buffer);
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                sprintf(tx_buffer, "No quedan mas cartas sobre la mesa\n");
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                sprintf(tx_buffer, "Es tu turno, %s\n", jugadores[player_number].nombre);
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                sprintf(tx_buffer, "Tenés que descartar\n");
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                strcpy(rx_buffer, "D");
                            }
                            do
                            {
                                restart = 0;
                                if (rx_buffer[0] == 'L')
                                {
                                    // Carta sobre la mesa (a, b, c, d)
                                    suma = 0;
                                    for (int i = 0; i < 40; i++)
                                    {
                                        mano_temp[i] = jugadores[player_number].mano[i];
                                        mesa_temp[i] = mesa[i];
                                        mazo_temp[i] = 0;
                                    }

                                    switch (contar_cartas(mano_temp))
                                    {
                                    case 1:
                                        send(socket_con, "Tu carta (a): ", 15, 0);
                                        break;
                                    case 2:
                                        send(socket_con, "Tu carta (a o b): ", 19, 0);
                                        break;
                                    default:
                                        send(socket_con, "Tu carta (a, b o c): ", 22, 0);
                                        break;
                                    }
                                    recv(socket_con, rx_buffer, 1024, 0);
                                    while (!(rx_buffer[0] >= 'a' && rx_buffer[0] <= 'a' + contar_cartas(mano_temp) - 1))
                                    {
                                        send(socket_con, "Ingrese una opción válida: ", 29, 0);
                                        recv(socket_con, rx_buffer, 1024, 0);
                                    }
                                    *carta_jugada = (int)rx_buffer[0] - 'a' + 1;
                                    for (int i = 0; i < 40; i++)
                                    {
                                        if (jugadores[player_number].mano[i] == 1)
                                        {
                                            *carta_jugada = *carta_jugada - 1;
                                            if (*carta_jugada == 0)
                                            {
                                                *carta_jugada = (i % 10) + 1;
                                                suma += *carta_jugada;
                                                mazo_temp[i] = 1;
                                                mano_temp[i] = 0;
                                                break;
                                            }
                                        }
                                    }
                                    while (suma < 15 && contar_cartas(mesa_temp) > 0)
                                    {
                                        mano_a_string(mesa_temp, tx_buffer);
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        sprintf(tx_buffer, "Carta sobre la mesa (a, ");
                                        for (int i = 1; i < contar_cartas(mesa_temp); i++)
                                        {
                                            sprintf(rx_buffer, "%c, ", 'a' + i);
                                            strcat(tx_buffer, rx_buffer);
                                        }
                                        tx_buffer[strlen(tx_buffer) - 2] = '\0';
                                        strcat(tx_buffer, "): ");
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        recv(socket_con, rx_buffer, 1024, 0);
                                        while (!((int)rx_buffer[0] >= 'a' && (int)rx_buffer[0] <= 'a' + contar_cartas(mesa_temp) - 1))
                                        {
                                            send(socket_con, "Ingrese una opción válida: ", 29, 0);
                                            recv(socket_con, rx_buffer, 1024, 0);
                                        }
                                        *carta_jugada = (int)rx_buffer[0] - 'a' + 1;
                                        for (int i = 0; i < 40; i++)
                                        {
                                            if (mesa_temp[i] == 1)
                                            {
                                                *carta_jugada = *carta_jugada - 1;
                                                if (*carta_jugada == 0)
                                                {
                                                    *carta_jugada = (i % 10) + 1;
                                                    suma += *carta_jugada;
                                                    mazo_temp[i] = 1;
                                                    mesa_temp[i] = 0;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    if (suma != 15)
                                    {
                                        if (suma > 15)
                                        {
                                            sprintf(tx_buffer, "La suma de las cartas es mayor a 15, comenzá denuevo\n");
                                        }
                                        else
                                        {
                                            sprintf(tx_buffer, "No hay más cartas sobre la mesa, comenzá denuevo\n");
                                        }
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        restart = 1;

                                        sprintf(tx_buffer, "Levantás (L) o descartás (D): ");
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        recv(socket_con, rx_buffer, 1024, 0);
                                        while (rx_buffer[0] != 'L' && rx_buffer[0] != 'D')
                                        {
                                            send(socket_con, "Ingrese una opción válida: ", 29, 0);
                                            recv(socket_con, rx_buffer, 1024, 0);
                                        }
                                    }
                                    else
                                    {
                                        // seteo la mano del jugador, la mesa y el mazo del jugador con las variables temporales
                                        for (int i = 0; i < 40; i++)
                                        {
                                            jugadores[player_number].mano[i] = mano_temp[i];
                                            mesa[i] = mesa_temp[i];
                                            jugadores[player_number].mazo[i] += mazo_temp[i];
                                        }
                                        if (contar_cartas(mesa) == 0)
                                        {
                                            sprintf(tx_buffer, "Armaste una escoba, llevas %d escobas en total\n", ++jugadores[player_number].escobas);
                                            send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        }

                                        for (int i = 0; i < total_players; i++)
                                        {
                                            jugadores[i].ultimo_en_levantar = 0;
                                        }
                                        jugadores[player_number].ultimo_en_levantar = 1;

                                        sprintf(tx_buffer, "Armaste el siguiente juego:\n");
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        mano_a_string(mazo_temp, tx_buffer);
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                    }
                                }
                            } while (restart);
                            restart = 0;

                            if (rx_buffer[0] == 'D')
                            {
                                // Tu carta (a, b o c)
                                switch (contar_cartas(jugadores[player_number].mano))
                                {
                                case 1:
                                    rx_buffer[0] = 'a';
                                    break;
                                case 2:
                                    send(socket_con, "Tu carta (a o b): ", 19, 0);
                                    recv(socket_con, rx_buffer, 1024, 0);
                                    break;
                                default:
                                    send(socket_con, "Tu carta (a, b o c): ", 22, 0);
                                    recv(socket_con, rx_buffer, 1024, 0);
                                    break;
                                }
                                while (!(rx_buffer[0] >= 'a' && rx_buffer[0] <= 'a' + contar_cartas(jugadores[player_number].mano) - 1))
                                {
                                    send(socket_con, "Ingrese una opción válida: ", 29, 0);
                                    recv(socket_con, rx_buffer, 1024, 0);
                                }
                                *carta_jugada = (int)rx_buffer[0] - 'a' + 1;
                                for (int i = 0; i < 40; i++)
                                {
                                    if (jugadores[player_number].mano[i] == 1)
                                    {
                                        *carta_jugada = *carta_jugada - 1;
                                        if (*carta_jugada == 0)
                                        {
                                            *carta_jugada = i;
                                            jugadores[player_number].mano[i] = 0;
                                            mesa[i] = 1;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (vuelta == 2 && player_number == 0 && contar_cartas(mazo) < total_players * 3)
                            {
                                *fin = 1;
                            }
                            for (int i = 0; i < total_players - 1; i++)
                            {
                                sem_post(0);
                            }
                        }
                        else
                        {
                            // ACA ESPERA A QUE EL JUGADOR DE TURNO JUEGUE
                            sprintf(tx_buffer, "Las cartas sobre la mesa son:\n");
                            send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                            mano_a_string(mesa, tx_buffer);
                            send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                            sprintf(tx_buffer, "Espero la jugada de %s\n", jugadores[turno].nombre);
                            send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                            sem_wait(0);
                            if (contar_cartas(mazo_temp) > 0)
                            {
                                if (contar_cartas(mesa) > 0 || *fin == 1)
                                {
                                    sprintf(tx_buffer, "%s armó el siguiente juego:\n", jugadores[turno].nombre);
                                }
                                else
                                {
                                    sprintf(tx_buffer, "%s armó el siguiente juego y se llevó una escoba:\n", jugadores[turno].nombre);
                                }
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                mano_a_string(mazo_temp, tx_buffer);
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                            }
                            else
                            {
                                sprintf(tx_buffer, "%s descartó la siguiente carta: ", jugadores[turno].nombre);
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                for (int i = 0; i < 40; i++)
                                {
                                    mazo_temp[i] = 0;
                                }
                                mazo_temp[*carta_jugada] = 1;
                                mano_a_string(mazo_temp, tx_buffer);
                                // Debo remover de tx_buffer el (a) del principio
                                send(socket_con, tx_buffer + 14, strlen(tx_buffer) - 14, 0);
                            }
                        }
                        turno++;
                    }
                    turno = 0;
                    vuelta++;
                }
                vuelta = 0;
            }

            // Las cartas sobrantes se las lleva el último jugador que levantó
            if (jugadores[player_number].ultimo_en_levantar == 1)
            {
                printf("[*] %s se lleva las cartas sobrantes\n", jugadores[player_number].nombre);
                sprintf(tx_buffer, "Te llevaste las cartas sobrantes\n");
                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                for (int i = 0; i < 40; i++)
                {
                    jugadores[player_number].mazo[i] += mesa[i];
                    mesa[i] = 0;
                }
                for (int i = 0; i < total_players - 1; i++)
                {
                    sem_post(0);
                }
            }
            else
            {
                for (int i = 0; i < player_number; i++)
                {
                    if (jugadores[i].ultimo_en_levantar == 1)
                    {
                        sprintf(tx_buffer, "%s se llevó las cartas sobrantes\n", jugadores[i].nombre);
                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                        break;
                    }
                }
                sem_wait(0);
            }

            // Elección del ganador
            int ganador;
            int valido;
            // Las escobas valen un punto cada una.                 LISTO
            // Si un jugador tiene todos los oros suma 2 puntos.    LISTO
            // Quien tenga la mayoria de los oros suma 1 punto.     LISTO
            // Quien tenga el 7 de oro suma 1 punto.                LISTO
            // Si alguien tiene todos los 7 gana 2 puntos.          LISTO
            // Por tener la mayoria de los 7 se suma 1 punto.       LISTO
            // Por tener la mayoria de las cartas se gana 1 punto.  LISTO

            // El jugador 0 es quien calcula los puntajes

            if (player_number == 0)
            {
                // Escobas
                for (int i = 0; i < total_players; i++)
                {
                    jugadores[i].puntos = jugadores[i].escobas;
                    printf("\033[0;35m[*]\033[0m %s tiene %d escobas (%d puntos)\n", jugadores[i].nombre, jugadores[i].escobas, jugadores[i].escobas);
                }

                // Todos los oros
                for (int i = 0; i < total_players; i++)
                {
                    valido = 1;
                    for (int j = 0; j < 10; j++)
                    {
                        if (jugadores[i].mazo[j] == 0)
                        {
                            valido = 0;
                        }
                    }
                    if (valido)
                    {
                        jugadores[i].puntos = jugadores[i].puntos + 2;
                        printf("\033[0;35m[*]\033[0m %s tiene todos los oros (2 puntos)\n", jugadores[i].nombre);
                        break;
                    }
                }

                // Mayoria de oros
                ganador = 0;
                // Si valido es igual a 0, significa que nadie tiene todos los oros
                if (valido == 0)
                {
                    for (int i = 0; i < total_players; i++)
                    {
                        // Utilizo valido como variable auxiliar para contar la cantidad maxima de oros que alguien tenga
                        valido = 0;
                        for (int j = 0; j < 10; j++)
                        {
                            if (jugadores[i].mazo[j] == 1)
                            {
                                valido++;
                            }
                        }
                        // Si un jugador tiene mas oros que el anterior, se guarda el nuevo maximo en ganador
                        if (valido > ganador)
                        {
                            ganador = valido;
                        }
                    }
                    // Al finalizar, se le asigna 1 punto a cada jugador que tenga "ganador" oros
                    for (int i = 0; i < total_players; i++)
                    {
                        valido = 0;
                        for (int j = 0; j < 10; j++)
                        {
                            if (jugadores[i].mazo[j] == 1)
                            {
                                valido++;
                            }
                        }
                        if (valido == ganador)
                        {
                            jugadores[i].puntos = jugadores[i].puntos + 1;
                            printf("\033[0;35m[*]\033[0m %s tiene la mayor cantidad de oros (1 punto)\n", jugadores[i].nombre);
                        }
                    }
                }

                // 7 de oros
                for (int i = 0; i < total_players; i++)
                {
                    if (jugadores[i].mazo[6] == 1)
                    {
                        jugadores[i].puntos = jugadores[i].puntos + 1;
                        printf("\033[0;35m[*]\033[0m %s tiene el 7 de oro (1 punto)\n", jugadores[i].nombre);
                        break;
                    }
                }

                // Todos los 7s
                for (int i = 0; i < total_players; i++)
                {
                    valido = 1;
                    for (int j = 6; j < 40; j = j + 10)
                    {
                        if (jugadores[i].mazo[j] == 0)
                        {
                            valido = 0;
                        }
                    }
                    if (valido)
                    {
                        jugadores[i].puntos = jugadores[i].puntos + 2;
                        printf("\033[0;35m[*]\033[0m %s tiene todos los sietes (2 puntos)\n", jugadores[i].nombre);
                        break;
                    }
                }

                // Mayoria de sietes
                ganador = 0;
                if (valido == 0)
                {
                    for (int i = 0; i < total_players; i++)
                    {
                        // Utilizo valido como variable auxiliar para contar la cantidad maxima de sietes que alguien tenga
                        valido = 0;
                        for (int j = 6; j < 40; j = j + 10)
                        {
                            if (jugadores[i].mazo[j] == 1)
                            {
                                valido++;
                            }
                        }
                        // Si un jugador tiene mas sietes que el anterior, se guarda el nuevo maximo en ganador
                        if (valido > ganador)
                        {
                            ganador = valido;
                        }
                    }
                    // Al finalizar, se le asigna 1 punto a cada jugador que tenga "ganador" sietes
                    for (int i = 0; i < total_players; i++)
                    {
                        valido = 0;
                        for (int j = 6; j < 40; j = j + 10)
                        {
                            if (jugadores[i].mazo[j] == 1)
                            {
                                valido++;
                            }
                        }
                        if (valido == ganador)
                        {
                            jugadores[i].puntos = jugadores[i].puntos + 1;
                            printf("\033[0;35m[*]\033[0m %s tiene la mayor cantidad de sietes (1 punto)\n", jugadores[i].nombre);
                        }
                    }
                }

                // Mayoria de cartas
                ganador = 0;
                for (int i = 0; i < total_players; i++)
                {
                    // Utilizo valido como variable auxiliar para contar la cantidad de cartas que alguien tenga
                    valido = contar_cartas(jugadores[i].mazo);
                    // Si un jugador tiene mas cartas que el anterior, se guarda el nuevo maximo en ganador
                    if (valido > ganador)
                    {
                        ganador = valido;
                    }
                }
                // Al finalizar, se le asigna 1 punto a cada jugador que tenga "ganador" cartas
                for (int i = 0; i < total_players; i++)
                {
                    valido = contar_cartas(jugadores[i].mazo);
                    if (valido == ganador)
                    {
                        jugadores[i].puntos = jugadores[i].puntos + 1;
                        printf("\033[0;35m[*]\033[0m %s tiene la mayor cantidad de cartas (1 punto)\n", jugadores[i].nombre);
                    }
                }

                for (int i = 0; i < total_players - 1; i++)
                {
                    sem_post(2);
                }
            }
            else
            {
                sem_wait(2);
            }

            ganador = 0;
            for (int i = 1; i < total_players; i++)
            {
                if (jugadores[i].puntos > jugadores[ganador].puntos)
                {
                    ganador = i;
                }
            }

            /*
            Imprimir el siguiente texto en cada jugador:

            Se terminó el juego. Resultados:
            Gerardo obtuvo las siguientes cartas y 2 escobas
                Oros: 1, 3, 5, 7, sota, rey
                Copas: 2, 5, 6, 7, sota, caballo
                Espadas: 1, 2, 4, 5, caballo
                Bastos: 1, 2, 6, caballo, rey
            Enrique obtuvo las siguientes cartas
                Oros: 2, 4, 6, caballo
                Copas: 1, 3, 4, rey
                Espadas: 3, 6, 7, sota, rey
                Bastos; 3, 4, 5, 7, sota*/

            sprintf(tx_buffer, "Se terminó el juego. Resultados:\n");
            send(socket_con, tx_buffer, strlen(tx_buffer), 0);
            for (int i = 0; i < total_players; i++)
            {
                sprintf(tx_buffer, "%s obtuvo las siguientes cartas y %d escobas\n", jugadores[i].nombre, jugadores[i].escobas);
                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                for (int j = 0; j < 40; j = j + 10)
                {
                    switch (j)
                    {
                    case 0:
                        sprintf(tx_buffer, "\t\033[0;33mOros:   \033[0m  ");
                        break;
                    case 10:
                        sprintf(tx_buffer, "\t\033[0;31mCopas:  \033[0m  ");
                        break;
                    case 20:
                        sprintf(tx_buffer, "\t\033[0;36mEspadas:\033[0m  ");
                        break;
                    case 30:
                        sprintf(tx_buffer, "\t\033[0;32mBastos: \033[0m  ");
                        break;
                    }
                    for (int k = 1; k < 8; k++)
                    {
                        if (jugadores[i].mazo[j + k - 1] == 1)
                        {
                            sprintf(rx_buffer, "%d, ", k);
                            strcat(tx_buffer, rx_buffer);
                        }
                    }
                    for (int k = 8; k < 11; k++)
                    {
                        if (jugadores[i].mazo[j + k - 1] == 1)
                        {
                            sprintf(rx_buffer, "%d, ", k + 2);
                            strcat(tx_buffer, rx_buffer);
                        }
                    }
                    // remover los ultimos dos caracteres y agregar un salto de linea
                    tx_buffer[strlen(tx_buffer) - 2] = '\0';
                    strcat(tx_buffer, "\n");
                    send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                }
            }

            sprintf(tx_buffer, "El ganador es %s con %d puntos y %d escobas!\n", jugadores[ganador].nombre, jugadores[ganador].puntos, jugadores[ganador].escobas);
            send(socket_con, tx_buffer, strlen(tx_buffer), 0);

            for (int i = 0; i < total_players; i++)
            {
                if (i != ganador)
                {
                    sprintf(tx_buffer, "%s hizo %d puntos.\n", jugadores[i].nombre, jugadores[i].puntos);
                    send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                }
            }

            if (player_number == 0)
            {
                printf("[*] El ganador es %s con %d escobas y %d cartas en el mazo\n", jugadores[ganador].nombre, jugadores[ganador].escobas, contar_cartas(jugadores[ganador].mazo));
            }

            close(socket_con);
            exit(0);
        }

        player_number++;
        if (player_number == total_players)
        {
            puts("\033[0;32m[+]\033[0m Todos los jugadores estan conectados");
            break;
        }
    }

    close(server_fd);
    for (int i = 0; i < total_players; i++)
    {
        waitpid(PID[i], NULL, 0);
    }

    shmdt(jugadores);
    shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);
    shmctl(shmid3, IPC_RMID, NULL);
    shmctl(shmid4, IPC_RMID, NULL);
    shmctl(shmid5, IPC_RMID, NULL);
    shmctl(shmid6, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, arg);

    return 0;
}
