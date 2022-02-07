#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define PORT 8080

int main(int argc, char const *argv[])
{
    int client_fd;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    FILE *fd;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Error en la creación del socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error de conexión");
        exit(EXIT_FAILURE);
    }

    recv(client_fd, buffer, 1024, 0);
    printf("%s\n", buffer);
    fgets(buffer, 40, stdin);
    while (1)
    {
        if (strncmp(buffer, "archivo ", 8) != 0)
        {
            printf("Ingrese un comando válido\n");
            fgets(buffer, 40, stdin);
            continue;
        }
        char *token = strchr(buffer, ' ');
        token++;
        if (strlen(token) == 1)
        {
            printf("Ingrese un nombre para el archivo\n");
            fgets(buffer, 40, stdin);
            continue;
        }
        token[strlen(token) - 1] = '\0';
        if (strlen(token) > 40)
        {
            printf("El nombre del archivo es muy largo\n");
            fgets(buffer, 40, stdin);
            continue;
        }
        char bad_chars[] = "!@%^*~|";
        for (int i = 0; i < strlen(bad_chars); i++)
        {
            if (strchr(token, bad_chars[i]) != NULL)
            {
                printf("Nombre del archivo inválido\n");
                fgets(buffer, 40, stdin);
                continue;
            }
        }
        if (!(fd = fopen(token, "rb")))
        {
            printf("El archivo no existe\n");
            fgets(buffer, 40, stdin);
            continue;
        }
        break;
    }
    send(client_fd, buffer, 1024, 0);

    do
    {
        printf("Ingrese el tamaño del archivo a enviar: ");
        scanf("%s", buffer);
    } while (!atoi(buffer));
    send(client_fd, buffer, 1024, 0);

    // enviar archivo
    int filesize = 0;
    int n;
    while ((n = fread(buffer, sizeof(char), 1024, fd)) != 0)
    {
        send(client_fd, buffer, n, 0);
        filesize += n;
    }
    fclose(fd);
    
    // Recibir confirmación
    memset(buffer, 0, 1024);
    recv(client_fd, buffer, 1024, 0);
    printf("%s\n", buffer);

    return 0;
}