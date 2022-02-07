#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
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
    struct tm *starttime, *endtime;

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

    while (1)
    {
        if ((socket_con = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Error de conexión");
            exit(EXIT_FAILURE);
        }
        printf("Conexión entrante desde %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        time(&rawtime);
        starttime = localtime(&rawtime);
        
        int bytesrecv = 0;
        int bytessent = 0;

        PID = fork();
        if (PID == 0)
        {
            close(server_fd);
            bytessent += send(socket_con, "listo", 6, 0);

            // Espera el comando archivo <nombre del archivo>
            bytesrecv += recv(socket_con, buffer, 1024, 0);
            char *filename = strtok(buffer, " ");
            filename = strtok(NULL, " ");

            // Espera el tamaño del archivo
            bytesrecv += recv(socket_con, buffer, 1024, 0);
            int expectedfilesize = atoi(buffer);

            fd = fopen(filename, "wb");
            int filesize = 0;
            while (filesize < expectedfilesize)
            {
                // BUG: Si se pone un tamaño esperado mayor al real se cuelga
                int n = recv(socket_con, buffer, 1024, 0);
                fwrite(buffer, sizeof(char), n, fd);
                filesize += n;
            }
            fclose(fd);

            memset(buffer, 0, 1024);
            sprintf(buffer, "Archivo %s completo, tamaño declarado %d bytes, tamaño real %d bytes.", filename, expectedfilesize, filesize);
            printf("%s\n", buffer);
            bytessent += send(socket_con, buffer, strlen(buffer), 0);

            time(&rawtime);
            endtime = localtime(&rawtime);
            log_fd = fopen("log.csv", "w");
            fprintf(log_fd, "%s:%d,%d,%d,%d,%s,%s\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port), filesize, bytessent, bytesrecv + filesize, asctime(starttime), asctime(endtime));
            fclose(log_fd);
            
            close(socket_con);
            break;
        }
    }

    return 0;
}