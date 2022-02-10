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
    int mazo[40];
    int mano[40];
} jugador;

void reiniciar_jugador(jugador jugador);
void imprimir_mazo(int *mazo);
void imprimir_mano(int mano[40]);
void reiniciar_mazo(int *mazo);
int repartir(int *mazo_fuente, int mazo_destino[40]);
int repartir3(int *mazo_fuente, int mazo_destino[40]);
void repartir_mesa(int *mazo_fuente, int *mazo_destino);
int contar_cartas(int *mazo);
void mano_a_string(int mano[40], char *string);

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

    key_t semkey = ftok("/bin/ls", SEM_KEY);
    union semun arg;
    struct sembuf operacion;
    operacion.sem_flg = 0;

    int semid = semget(semkey, 2, IPC_CREAT | 0666);

    arg.val = 0;
    semctl(semid, 0, SETVAL, arg);
    arg.val = 1;
    semctl(semid, 1, SETVAL, arg);

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
    imprimir_mazo(mesa);
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
        for (int i = 0; i < 40; i++)
        {
            jugadores[player_number].mazo[i] = 0;
            jugadores[player_number].mano[i] = 0;
        }

        PID[player_number] = fork();
        if (PID[player_number] == 0)
        {
            close(server_fd);
            send(socket_con, "Decime tu nombre: ", 18, 0);
            recv(socket_con, rx_buffer, 1024, 0);
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
            int fin = 0;
            int vuelta = 0;
            int k = 'a';
            int suma = 0;

            while (!fin)
            {
                sem_wait(1);
                repartir3(mazo, jugadores[player_number].mano);
                printf("[*] Mano del jugador %d:\n", player_number + 1);
                mano_a_string(jugadores[player_number].mano, tx_buffer);
                printf("%s", tx_buffer);
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
                            int carta_jugada;
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
                                sprintf(tx_buffer, "Levantás (L) o descartás (D)\n");
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
                                sprintf(tx_buffer, "\nTus cartas son:\n");
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                mano_a_string(jugadores[player_number].mano, tx_buffer);
                                send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                sprintf(tx_buffer, "Las cartas sobre la mesa son:\nNinguna\n");
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
                                    int mano_temp[40], mesa_temp[40], mazo_temp[40];
                                    for (int i = 0; i < 40; i++)
                                    {
                                        mano_temp[i] = jugadores[player_number].mano[i];
                                        mesa_temp[i] = mesa[i];
                                        mazo_temp[i] = 0;
                                    }

                                    switch (contar_cartas(mano_temp))
                                    {
                                    case 1:
                                        send(socket_con, "Tu carta (a)\n", 14, 0);
                                        break;
                                    case 2:
                                        send(socket_con, "Tu carta (a o b)\n", 18, 0);
                                        break;
                                    default:
                                        send(socket_con, "Tu carta (a, b o c)\n", 21, 0);
                                        break;
                                    }
                                    recv(socket_con, rx_buffer, 1024, 0);
                                    while (rx_buffer[0] != 'a' && rx_buffer[0] != 'b' && rx_buffer[0] != 'c')
                                    {
                                        send(socket_con, "Ingrese una opción válida: ", 29, 0);
                                        recv(socket_con, rx_buffer, 1024, 0);
                                    }
                                    carta_jugada = (int)rx_buffer[0] - k + 1;
                                    for (int i = 0; i < 40; i++)
                                    {
                                        if (jugadores[player_number].mano[i] == 1)
                                        {
                                            carta_jugada--;
                                            if (carta_jugada == 0)
                                            {
                                                carta_jugada = (i % 10) + 1;
                                                suma += carta_jugada;
                                                mazo_temp[i] = 1;
                                                mano_temp[i] = 0;
                                                break;
                                            }
                                        }
                                    }
                                    while (suma < 15)
                                    {
                                        mano_a_string(mesa_temp, tx_buffer);
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        int cantidad_en_mesa = contar_cartas(mesa_temp);
                                        sprintf(tx_buffer, "Carta sobre la mesa (a, ");
                                        for (int i = 1; i < cantidad_en_mesa; i++)
                                        {
                                            sprintf(rx_buffer, "%c ,", 'a' + i);
                                            strcat(tx_buffer, rx_buffer);
                                        }
                                        tx_buffer[strlen(tx_buffer) - 1] = '\0';
                                        strcat(tx_buffer, ")\n");
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        recv(socket_con, rx_buffer, 1024, 0);
                                        while (!((int)rx_buffer[0] >= 97 && (int)rx_buffer[0] <= (97 + cantidad_en_mesa - 1)))
                                        {
                                            send(socket_con, "Ingrese una opción válida: ", 29, 0);
                                            recv(socket_con, rx_buffer, 1024, 0);
                                        }
                                        carta_jugada = (int)rx_buffer[0] - k + 1;
                                        for (int i = 0; i < 40; i++)
                                        {
                                            if (mesa_temp[i] == 1)
                                            {
                                                carta_jugada--;
                                                if (carta_jugada == 0)
                                                {
                                                    carta_jugada = (i % 10) + 1;
                                                    suma += carta_jugada;
                                                    mazo_temp[i] = 1;
                                                    mesa_temp[i] = 0;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    if (suma > 15)
                                    {
                                        sprintf(tx_buffer, "La suma de las cartas es mayor a 15, comenzá denuevo\n");
                                        send(socket_con, tx_buffer, strlen(tx_buffer), 0);
                                        restart = 1;

                                        sprintf(tx_buffer, "Levantás (L) o descartás (D)\n");
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
                                            jugadores[player_number].mazo[i] = mazo_temp[i];
                                        }
                                        sprintf(tx_buffer, "Armaste una escoba, llevas %d escobas en total\n", ++jugadores[player_number].escobas);
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
                                    send(socket_con, "Tu carta (a o b)\n", 18, 0);
                                    recv(socket_con, rx_buffer, 1024, 0);
                                    break;
                                default:
                                    send(socket_con, "Tu carta (a, b o c)\n", 21, 0);
                                    recv(socket_con, rx_buffer, 1024, 0);
                                    break;
                                }
                                while (rx_buffer[0] != 'a' && rx_buffer[0] != 'b' && rx_buffer[0] != 'c')
                                {
                                    send(socket_con, "Ingrese una opción válida: ", 29, 0);
                                    recv(socket_con, rx_buffer, 1024, 0);
                                }
                                carta_jugada = (int)rx_buffer[0] - k + 1;
                                for (int i = 0; i < 40; i++)
                                {
                                    if (jugadores[player_number].mano[i] == 1)
                                    {
                                        carta_jugada--;
                                        if (carta_jugada == 0)
                                        {
                                            carta_jugada == i;
                                            jugadores[player_number].mano[i] = 0;
                                            mesa[i] = 1;
                                            break;
                                        }
                                    }
                                }
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
                        }
                        turno++;
                    }
                    turno = 0;
                    vuelta++;
                }
                vuelta = 0;

                if (contar_cartas(mazo) < (player_number * 3))
                {
                    fin = 1;
                }
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
    semctl(semid, 0, IPC_RMID, arg);

    return 0;
}

int repartir(int *mazo_fuente, int mazo_destino[40])
{
    if (contar_cartas(mazo_fuente) < 1)
    {
        return 1;
    }

    srand(time(0));
    int carta = rand() % 10;
    int palo = rand() % 4;
    while (mazo_fuente[palo * 10 + carta] == 0)
    {
        carta = rand() % 10;
        palo = rand() % 4;
    }
    mazo_fuente[palo * 10 + carta] = 0;
    mazo_destino[palo * 10 + carta] = 1;

    return 0;
}

int repartir3(int *mazo_fuente, int mazo_destino[40])
{
    if (contar_cartas(mazo_fuente) < 3)
    {
        return 1;
    }

    repartir(mazo_fuente, mazo_destino);
    repartir(mazo_fuente, mazo_destino);
    repartir(mazo_fuente, mazo_destino);

    return 0;
}

void repartir_mesa(int *mazo_fuente, int *mazo_destino)
{
    srand(time(0));
    int carta = rand() % 10;
    int palo = rand() % 4;
    while (mazo_fuente[palo * 10 + carta] == 0)
    {
        carta = rand() % 10;
        palo = rand() % 4;
    }
    mazo_fuente[palo * 10 + carta] = 0;
    mazo_destino[palo * 10 + carta] = 1;
}

int contar_cartas(int *mazo)
{
    int cartas = 0;
    for (int i = 0; i < 40; i++)
    {
        if (mazo[i] == 1)
        {
            cartas++;
        }
    }
    return cartas;
}

void imprimir_mazo(int *mazo)
{
    printf("\033[0;32m\t1 2 3 4 5 6 7 S C R\033[0m\n");
    printf("Oro:\t");
    for (int j = 0; j < 10; j++)
    {
        printf("%d ", mazo[j]);
    }
    printf("\n");
    printf("Copa:\t");
    for (int j = 0; j < 10; j++)
    {
        printf("%d ", mazo[10 + j]);
    }
    printf("\n");
    printf("Espada:\t");
    for (int j = 0; j < 10; j++)
    {
        printf("%d ", mazo[20 + j]);
    }
    printf("\n");
    printf("Basto:\t");
    for (int j = 0; j < 10; j++)
    {
        printf("%d ", mazo[30 + j]);
    }
    printf("\n");
}

void imprimir_mano(int mano[40])
{
    printf("\033[0;32m\t1 2 3 4 5 6 7 S C R\033[0m\n");
    printf("Oro:\t");
    for (int j = 0; j < 10; j++)
    {
        printf("%d ", mano[j]);
    }
    printf("\n");
    printf("Copa:\t");
    for (int j = 0; j < 10; j++)
    {
        printf("%d ", mano[10 + j]);
    }
    printf("\n");
    printf("Espada:\t");
    for (int j = 0; j < 10; j++)
    {
        printf("%d ", mano[20 + j]);
    }
    printf("\n");
    printf("Basto:\t");
    for (int j = 0; j < 10; j++)
    {
        printf("%d ", mano[30 + j]);
    }
    printf("\n");
}

void mano_a_string(int mano[40], char *string)
{
    char buffer[24];
    char palo[7];
    int carta;
    int k = 'a';
    for (int i = 0; i < 40; i++)
    {
        if (mano[i] == 1)
        {
            switch (i / 10)
            {
            case 0:
                strcpy(palo, "Oro");
                break;
            case 1:
                strcpy(palo, "Copa");
                break;
            case 2:
                strcpy(palo, "Espada");
                break;
            case 3:
                strcpy(palo, "Basto");
                break;
            }
            carta = (i % 10) + 1;
            if (carta == 8 || carta == 9 || carta == 10)
            {
                carta = carta + 2;
            }
            if (k == 'a')
            {
                sprintf(buffer, "(%c) %d de %s ", k, carta, palo);
                strcpy(string, buffer);
            }
            else
            {
                sprintf(buffer, "(%c) %d de %s ", k, carta, palo);
                strcat(string, buffer);
            }
            k++;
        }
    }
    strcat(string, "\n");
}