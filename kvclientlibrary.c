#define PAD_SYM "*"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include "kvclientlibrary.h"
//  FUNCTION DELCLARAITON

int put(char *key, char *value, char *error, int sock_fd)
{

    char *packet = (char *)malloc(514 * sizeof(char));
    strcpy(packet, "2");
    strcat(packet, key);
    for (int i = 0; i < 256 - strlen(key); i++)
    {
        strcat(packet, PAD_SYM);
    }
    strcat(packet, value);
    for (int i = 0; i < 256 - strlen(value); i++)
    {
        strcat(packet, PAD_SYM);
    }
    packet[513] = '\0';
    int result = send(sock_fd, packet, 514,0);
    if (result < 0)
    {
        perror("Client Send Error");
        strcpy(error, "Client Failed To Send");
        return 240;
    }
    char *response = (char *)malloc(514 * sizeof(char));
    assert(response != NULL);
    if (recv(sock_fd, response, 514, 0) < 0) //Blocking Call
    {
        perror("No Response From The Server\n");
        strcpy(error, "No Response From The Server");
        return 240;
    }
    else
    {
        return 200;
    }
    free(response);
}

int get(char *key, char *value, char *error, int sock_fd)
{

    char *packet = (char *)malloc(514 * sizeof(char));
    strcpy(packet, "1");
    strcat(packet, key);
    for (int i = 0; i < 512 - strlen(key); i++)
    {
        strcat(packet, PAD_SYM);
    }
    packet[513] = '\0';
    int result = send(sock_fd, packet, 514,0);
    if (result < 0)
    {
        perror("Client Send Error");
        return -1;
    }
    char *response = (char *)malloc(514 * sizeof(char));
    assert(response != NULL);
    if (recv(sock_fd, response, 514, 0) < 0) //Blocking Call
    {
        perror("No Response From The Server\n");
        return -1;
    }
    else
    {
        char d;
        d = response[1];
        char *status = (char *)malloc(sizeof(char) * 257);
        status[0] = '\0';
        for (int i = 3, j = 0; (i <= strlen(response) && response[i] != '*'); i++, j++)
        {
            status[j] = response[i];
            status[j + 1] = '\0';
        }
        if (d == '0')
        {
            strcpy(value, status);
            return (200);
        }
        else
        {
            strcpy(error, status);
            return (240);
        }
        free(status);
    }
    free(response);
}

int del(char *key, char *error, int sock_fd)
{
    char *packet = (char *)malloc(514 * sizeof(char));
    strcpy(packet, "3");
    strcat(packet, key);
    for (int i = 0; i < 512 - strlen(key); i++)
    {
        strcat(packet, PAD_SYM);
    }
    packet[513] = '\0';
    int result = send(sock_fd, packet, 514,0);
    if (result < 0)
    {
        perror("Client Send Error");
        return -1;
    }
    char *response = (char *)malloc(514 * sizeof(char));
    assert(response != NULL);
    if (recv(sock_fd, response, 514, 0) < 0) //Blocking Call
    {
        perror("No Response From The Server\n");
        return -1;
    }
    else
    {
        char d;
        d = response[1];
        char *status = (char *)malloc(sizeof(char) * 257);
        status[0] = '\0';
        for (int i = 3, j = 0; (i <= strlen(response) && response[i] != '*'); i++, j++)
        {
            status[j] = response[i];
            status[j + 1] = '\0';
        }
        if (d == '0')
            return (200);
        else
        {
            strcpy(error, status);
            return (240);
        }
        free(status);
    }
    free(response);
}

void show_cache_data(int sock_fd)
{
    char *packet = (char *)malloc(514 * sizeof(char));
    strcpy(packet, "4");
    for (int i = 0; i < 512; i++)
    {
        strcat(packet, PAD_SYM);
    }
    packet[513] = '\0';
    int result = write(sock_fd, packet, strlen(packet) + 1);
    if (result < 0)
    {
        perror("Client Send Error");
        return;
    }
    char *response = (char *)malloc(514 * sizeof(char));
    assert(response != NULL);
    if (recv(sock_fd, response, 514, 0) < 0) //Blocking Call
    {
        perror("No Response From The Server\n");
        return;
    }
    else
    {
        printf("Respone is: %s\n", response);
    }
    free(response);
}

void show_file_db(int sock_fd)
{
    char *packet = (char *)malloc(514 * sizeof(char));
    strcpy(packet, "5");
    for (int i = 0; i < 512; i++)
    {
        strcat(packet, PAD_SYM);
    }
    packet[513] = '\0';
    int result = write(sock_fd, packet, strlen(packet) + 1);
    if (result < 0)
    {
        perror("Client Send Error");
        return;
    }
    char *response = (char *)malloc(514 * sizeof(char));
    assert(response != NULL);
    if (recv(sock_fd, response, 514, 0) < 0) //Blocking Call
    {
        perror("No Response From The Server\n");
        return;
    }
    else
    {
        printf("Respone is: %s\n", response);
    }
    free(response);
}