#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Funcion auxiliar. Imprime las cartas de un determinado mazo en formato de '0' y '1' de forma matricial
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

// Funcion auxiliar. Imprime una mano en formato de '0' y '1' de forma matricial
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

// Toma una carta aleatoria del mazo fuente y la pasa al mazo destino
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

// Repite 3 veces la funcion 'repartir'
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

// Misma funcion que 'repartir' pero es especifica para el mazo actual de la mesa
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

// Contar cartas de un determinado mazo (Usuario o mesa)
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

// Toma un determinado mazo y crea una cadena de texto con las cartas que contiene
void mano_a_string(int mano[40], char *string)
{
    char buffer[64];
    char palo[32];
    int carta;
    int k = 'a';
    strcpy(string, "Ninguna ");
    for (int i = 0; i < 40; i++)
    {
        if (mano[i] != 1)
            continue;
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
            sprintf(buffer, "\033[0;2m(%c)\033[0m %d de %s ", k, carta, palo);
            strcpy(string, buffer);
        }
        else
        {
            sprintf(buffer, "\033[0;2m(%c)\033[0m %d de %s ", k, carta, palo);
            strcat(string, buffer);
        }
        k++;
    }
    // remover el espacio del final
    string[strlen(string) - 1] = '\0';
    strcat(string, "\n");
}