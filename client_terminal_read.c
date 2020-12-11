#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include "kvclientlibrary.h"

char PORT[10];
void read_conf_file(char *path);

int main(int argc, char *argv[])
{
	int s, sock_fd;
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket Creation Error \n");
		return -1;
	}
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (argc != 2)
	{
		printf("Enter the path of the server.conf file\n");
		exit(1);
	}
	read_conf_file(argv[1]);
	s = getaddrinfo(NULL, PORT, &hints, &result);
	if (s != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(1);
	}

	int d = connect(sock_fd, result->ai_addr, result->ai_addrlen);
	printf("Connection Status: %d\n", d);
	char *key = (char *)malloc(257 * sizeof(char));
	char *value = (char *)malloc(257 * sizeof(char));
	char *error = (char *)malloc(257 * sizeof(char));
	char ins[3];
	char c;
	int resp;

	while (1)
	{
		value[0] = '\0';
		key[0] = '\0';
		error[0] = '\0';
		if (sock_fd != -1)
		{
			printf("\n> [I]: Insert a key/value pair to keystore.\n");
			printf("> [D]: Delete a key/value pair from keystore.\n");
			printf("> [L]: Get a value from the keystore.\n");
			printf("> [C]: Show Cache Content\n");
			printf("> [F]: Show File  Metadata Content\n");
			printf("> [Q]: Quit \n");
			fgets(ins, sizeof(ins), stdin);
			ins[1] = '\0';
			c = tolower(ins[0]);
			if (c == 'q')
			{
				printf("Exiting Now\n");
				close(sock_fd);
				exit(1);
			}
			switch (c)
			{
			case 'i':
				printf("................................\n");
				printf("Enter The Key:- ");
				fgets(key, 257, stdin); // read from stdin
				key[strlen(key) - 1] = '\0';
				printf("\n");
				printf("Enter The Value:- ");
				fgets(value, 257, stdin); // read from stdin
				value[strlen(value) - 1] = '\0';
				printf("\n");
				printf("................................\n");
				resp = put(key, value, error, sock_fd);
				if (resp == 240)
					printf("Response: %d %s\n", resp, error);
				else
					printf("Response: %d\n", resp);
				break;
			case 'd':
				printf("................................\n");
				printf("Enter The Key:- ");
				fgets(key, 257, stdin); // read from stdin
				key[strlen(key) - 1] = '\0';
				printf("\n");
				printf("................................\n");
				resp = del(key, error, sock_fd);
				if (resp == 240)
					printf("Response: %d %s\n", resp, error);
				else
					printf("Response: %d\n", resp);

				break;

			case 'l':
				printf("................................\n");
				printf("Enter The Key:- ");
				fgets(key, 257, stdin); // read from stdin
				key[strlen(key) - 1] = '\0';
				printf("\n");
				printf("................................\n");
				resp = get(key, value, error, sock_fd);
				if (resp == 240)
					printf("Response: %d %s\n", resp, error);
				else
					printf("Response: %d %s\n", resp, value);

				break;
			case 'c':
				show_cache_data(sock_fd);
				break;
			case 'f':
				show_file_db(sock_fd);
				break;
			default:
				printf("Invalid Input. Enter Again\n");
				break;
			}
		}
		else
		{
			printf("FD Error\n");
			exit(0);
		}
	}
	return 0;
}

void read_conf_file(char *path)
{
	FILE *fptr;
	if ((fptr = fopen(path, "r")) == NULL)
	{
		printf("Error! opening file");
		// Program exits if file pointer returns NULL.
		exit(1);
	}
	// reads text until newline is encountered
	fgets(PORT, sizeof(PORT), fptr);
	strtok(PORT, "\n");
	printf("\nInformation from the conf file: port number %s\n", PORT);
	fclose(fptr);
}