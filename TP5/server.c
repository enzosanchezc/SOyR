#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#define PORT 8080

int main(int argc, char const *argv[])
{
    int server_fd, socket_con;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    pid_t PID;

    FILE *fd;
    FILE *log_fd;

    time_t rawtime;
    char starttime[64], endtime[64];

    int child_count = 0;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Error en la creación del socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Error en la asignación del socket");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 10);

    while (10 - child_count)
    {
        if ((socket_con = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Error de conexión");
            exit(EXIT_FAILURE);
        }
        printf("Conexión entrante desde %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        child_count++;

        time(&rawtime);
        strcpy(starttime, asctime(localtime(&rawtime)));

        int bytesrecv = 0;
        int bytessent = 0;

        PID = fork();
        if (PID == 0)
        {
            close(server_fd);
            bytessent += send(socket_con, "Listo", 6, 0);

            // Espera el comando archivo <nombre del archivo>
            bytesrecv += recv(socket_con, buffer, 1024, 0);
            char *filename = strtok(buffer, " ");
            filename = strtok(NULL, " ");
            char _filename[strlen(filename)];
            strcpy(_filename, filename);

            // Espera el tamaño del archivo
            bytesrecv += recv(socket_con, buffer, 1024, 0);
            int expectedfilesize = atoi(buffer);

            fd = fopen(filename, "wb");
            int filesize = 0;
            int n;
            do
            {
                n = recv(socket_con, buffer, 1024, 0);
                fwrite(buffer, sizeof(char), n, fd);
                filesize += n;
            } while (n == 1024);
            fclose(fd);

            memset(buffer, 0, 1024);
            sprintf(buffer, "Archivo %s completo, tamaño declarado %d bytes, tamaño real %d bytes.", _filename, expectedfilesize, filesize);
            printf("%s\n", buffer);
            bytessent += send(socket_con, buffer, strlen(buffer), 0);

            time(&rawtime);
            strcpy(endtime, asctime(localtime(&rawtime)));

            endtime[strlen(endtime) - 1] = '\0';
            starttime[strlen(starttime) - 1] = '\0';

            log_fd = fopen("log.csv", "a");
            fprintf(log_fd, "%s:%d,%d,%d,%d,%s,%s\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port), filesize, bytessent, bytesrecv + filesize, starttime, endtime);
            fclose(log_fd);

            close(socket_con);
            return 0;
        }
    }

    // verificar que no queden procesos zombie
    for (int i = 0; i < child_count; i++)
    {
        wait(NULL);
    }

    return 0;
}