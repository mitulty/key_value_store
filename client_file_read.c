#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "kvclientlibrary.h"
#include <time.h>

char PORT[10];
void read_conf_file(char *path);

int main(int argc, char *argv[])
{
	int s,sock_fd;
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket Creation Error \n");
		return -1;
	}
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (argc != 4)
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
	int resp;

	char *operation = (char *)malloc(sizeof(char) * 7);

	FILE *fp;

	fp = fopen(argv[2], "r");
	if (fp == NULL)
	{
		printf("ERROR:File doesn't exist\n");
		return -1;
	}

	FILE *outFp;
	outFp = fopen(argv[3], "w+");
	clock_t tm;
	tm = clock();
	int inp;

	while (1)
	{
		if (sock_fd != -1)
		{
			value[0] = '\0';
			key[0] = '\0';
			error[0] = '\0';
			if (fscanf(fp, "%[^,],", operation) == EOF)
			{
				shutdown(sock_fd, SHUT_WR);
				close(sock_fd);
				sleep(1);
				fclose(fp);
				break;
			}
			inp = atoi(operation);
			if (inp == 2)
			{
				//put
				fscanf(fp, "%[^,],%[^\n]\n", key, value);
				//	printf(" Put %d %s %s %s\n", inp, key, value, argv[3]);
				resp = put(key, value, error, sock_fd);
				//	printf("Put Response %s\n", argv[3]);
				if (resp == 240)
					fprintf(outFp, "Response: %d %s\n", resp, error);
				else
					fprintf(outFp, "Response: %d\n", resp);
			}
			else if (inp == 1)
			{
				//get
				fscanf(fp, "%[^\n]\n", key);
				//	printf(" Get %d,%s %s\n", inp, key, argv[3]);
				resp = get(key, value, error, sock_fd);
				//	printf("Get Response %s\n", argv[3]);
				if (resp == 240)
					fprintf(outFp, "Response: %d %s\n", resp, error);
				else
					fprintf(outFp, "Response: %d %s\n", resp, value);
			}
			else if (inp == 3)
			{
				//del
				fscanf(fp, "%[^\n]\n", key);
				//	printf(" Del %d,%s %s\n", inp, key, argv[3]);
				resp = del(key, error, sock_fd);
				//	printf("Del Response %s\n", argv[3]);
				if (resp == 240)
					fprintf(outFp, "Response: %d %s\n", resp, error);
				else
					fprintf(outFp, "Response: %d\n", resp);
			}
			else
			{
				fprintf(outFp, "Invalid Choice\n");
			}
		}
		else
		{
			printf("FD Error\n");
			exit(0);
		}
	}
	tm = clock() - tm;
	double time_taken = ((double)tm) / CLOCKS_PER_SEC; // in seconds

	fprintf(outFp, "File took %f seconds to execute \n", time_taken);

	fclose(outFp);
	printf("Connection Closed For Thread %s\n", argv[3]);
	sleep(1);
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
