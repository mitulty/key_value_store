#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include "server.h"
#include "lfucache.h"
#include "kvstore.h"
#include "myqueue.h"

char PORT[10];
int CLIENTS_PER_THREAD;
int CACHE_SIZE;
char CACHE_REPLACEMENT[5];
int THREAD_POOL_SIZE_INITIAL;
int THREAD_POOL_GROWTH;
int NO_OF_THREADS = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int server_sock_fd;
thread_data *thread_pool_head = NULL, *thread_pool_tail = NULL;
int no_of_active_connections = 0;
void show_data();
int main(int argc, char *argv[])
{
    int s;
    int round = 0, f = 0, client_fd;
    int optval = 1;
    signal(SIGINT, siginthandler);
    if (argc != 2)
    {
        printf("Enter the path of the server.conf file\n");
        exit(1);
    }
    server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct linger sl;
    sl.l_onoff = 1;  /* non-zero value enables linger option in kernel */
    sl.l_linger = 0; /* timeout interval in seconds */
    if (setsockopt(server_sock_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) == -1)
        perror("setcockopt_1");
    if (setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
        perror("setsockopt__2");
    read_conf_file(argv[1]);
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    s = getaddrinfo(NULL, PORT, &hints, &result);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }
    if (bind(server_sock_fd, result->ai_addr, result->ai_addrlen) != 0)
    {
        perror("bind()");
        exit(1);
    }
    if (listen(server_sock_fd, 1000) != 0)
    {
        perror("listen()");
        exit(1);
    }
    init_threadpool();
    //Creating  Store
    init_store();
    // Creating Cache
    create_cache(CACHE_SIZE);
    // Creating Thread Pool
   // disply_cache_data();
    //show_data();
    printf("STARTED SERVER\n");
    pthread_t *thread_handles = (pthread_t *)malloc(THREAD_POOL_SIZE_INITIAL * (sizeof(pthread_t)));
    for (int thread = 0; thread < THREAD_POOL_SIZE_INITIAL; thread++)
    {
        int *arg = malloc(sizeof(*arg));
        if (arg == NULL)
        {
            fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
            exit(1);
        }
        *arg = thread;
        pthread_create(&thread_handles[thread], NULL, worker_thr, arg);
    }
    free(thread_handles);
    NO_OF_THREADS += THREAD_POOL_SIZE_INITIAL;
    printf("SERVER SETUP SUCCESSFULLY\n");
    thread_data *worker_thread_node_fd = NULL;
    while (1)
    {
        // show_conn();
        printf("\n------------------------------------------------------------\n");
        client_fd = accept(server_sock_fd, NULL, NULL); //Blocking Call
        printf("CONNECTION RECEIVED WITH CLIENT ID %d\n", client_fd);
        if (round == NO_OF_THREADS)
        {
            f = 1;
            //    printf("First Phase Cycle Complete\n");
        }
        if (round < NO_OF_THREADS && f == 0)
        {
            pthread_mutex_lock(&lock);
            worker_thread_node_fd = get_ds(round);
            worker_thread_node_fd->client_fd = client_fd;
           // printf("Entering in queue and waking up thread %p %d\n", worker_thread_node_fd, worker_thread_node_fd->thread_id);
            pthread_cond_signal(&worker_thread_node_fd->thread_cond_var);
            pthread_mutex_unlock(&lock);
            round++;
        }
        if (f == 1)
        {
            if (no_of_active_connections <= ((NO_OF_THREADS) * CLIENTS_PER_THREAD))
            {
                //    printf("Usage of thread %d for client %d\n", worker_thread_node_fd->thread_id, client_fd);
                pthread_mutex_lock(&lock);
                enqueue(client_fd);
                pthread_mutex_unlock(&lock);
            }
            else
            {
                //    printf("Full!!!!\n");
                f = 0;
                round = NO_OF_THREADS;
                extend_threadpool();
                show_data();
                pthread_t *thread_handles_add = (pthread_t *)malloc(THREAD_POOL_GROWTH * (sizeof(pthread_t)));
                for (int thread_add = NO_OF_THREADS; thread_add < THREAD_POOL_GROWTH + NO_OF_THREADS; thread_add++)
                {
                    int *arg = malloc(sizeof(*arg));
                    if (arg == NULL)
                    {
                        fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
                        exit(1);
                    }
                    *arg = thread_add;
                    pthread_create(&thread_handles_add[thread_add], NULL, worker_thr, arg);
                }
                sleep(1);
                NO_OF_THREADS += THREAD_POOL_GROWTH;
                free(thread_handles_add);
                pthread_mutex_lock(&lock);
                //                printf("Entering in queue and waking up thread %d\n", round);
                enqueue(client_fd);
                worker_thread_node_fd->no_of_connections++;
                pthread_cond_signal(&worker_thread_node_fd->thread_cond_var);
                pthread_mutex_unlock(&lock);
                round++;
            }
        }
    }
    return 0;
}

void *worker_thr(void *args)
{
    int thread_id = *((int *)args);
    free(args);
    thread_data *fd;
    fd = get_ds(thread_id);
    //  printf("Thread Created with thread id %d and connections %d\n", thread_id, fd->no_of_connections);
    int epoll_fd;
    if ((epoll_fd = epoll_create(CLIENTS_PER_THREAD)) < 0)
    {
        perror("epoll_create() failed");
        exit(1);
    }

    struct epoll_event epevent;
    epevent.events = EPOLLIN;
    int pclient_fd;
    int events_cnt;
    pthread_mutex_lock(&lock);
    while (fd->no_of_connections == 0)
    {
        pthread_cond_wait(&fd->thread_cond_var, &lock);
        fd->no_of_connections++;
    }
    pclient_fd = fd->client_fd;
    pthread_mutex_unlock(&lock);
    // printf("Received client %d for thread %d and No of Connections is %d\n", pclient_fd, thread_id, fd->no_of_connections);
    if (pclient_fd != -1)
    {
        epevent.data.fd = pclient_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pclient_fd, &epevent) < 0)
        {
            perror("epoll_ctl() failed in adding first client");
            fd->no_of_connections--;
        }
        else
        {
            pthread_mutex_lock(&lock);
            no_of_active_connections++;
            pthread_mutex_unlock(&lock);
            //printf("First New Connection Added %d for thread %p %d\n", pclient_fd, fd, thread_id);
        }
    }
    while (1)
    {
        if ((events_cnt = epoll_wait(epoll_fd, &epevent, CLIENTS_PER_THREAD, 100)) > 0)
        {
            //printf("After Epoll Wait %d\n", thread_id);
            for (int i = 0; i < events_cnt; i++)
            {
                if (client_request(epevent.data.fd) == -1)
                {
                    fprintf(stdout, "Connection Closed for Client Id %d\n", epevent.data.fd);
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epevent.data.fd, &epevent) == -1)
                        perror("epoll_ctl");
                    if (close(epevent.data.fd) == -1)
                        perror("close");
                    fd->no_of_connections--;
                    pthread_mutex_lock(&lock);

                    no_of_active_connections--;
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        if (fd->no_of_connections < CLIENTS_PER_THREAD)
        {
            pthread_mutex_lock(&lock);
            pclient_fd = dequeue();
            pthread_mutex_unlock(&lock);
            if (pclient_fd != -1)
            {
                epevent.data.fd = pclient_fd;
                epevent.events = EPOLLIN;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pclient_fd, &epevent) < 0)
                {
                    perror("epoll_ctl() failed in adding other new clients\n");
                    close(pclient_fd);
                }
                else
                {
                  //  printf("New Connection Added %d for thread %p %d\n", pclient_fd, fd, thread_id);
                    pthread_mutex_lock(&lock);
                    no_of_active_connections++;
                    pthread_mutex_unlock(&lock);
                    fd->no_of_connections++;
                }
            }
        }
    }
    return NULL;
}
int client_request(int client_fd)
{
    char buffer[514], c;
    int len, type;
    len = recv(client_fd, buffer, 514, 0); //Blocking Call
    //printf("Client %d Request Arrived of length %d\n", client_fd,len);
    char *return_buffer = (char *)malloc(514 * sizeof(char));
    return_buffer = "Done";
    if (len == -1)
    {
       // fprintf(stderr, "%s Error for Request of length %d for Client %d\n", strerror(errno), len, client_fd);
        return 0;
    }
    if (len == 0)
    {
       // printf("Quitting\n");
        return -1; //Quit
    }
    c = buffer[0];
    type = atoi(&c);
    char *key = (char *)malloc(sizeof(char) * 256);
    for (int i = 1; (i <= 256 && buffer[i] != '*'); i++)
    {
        c = buffer[i];
        strncat(key, &c, 1);
    }
    char *value = (char *)malloc(sizeof(char) * 256);
    for (int i = 257; (i <= 513 && buffer[i] != '*'); i++)
    {
        c = buffer[i];
        strncat(value, &c, 1);
    }
    if (type == 2)
        return_buffer = put_data(key, value);
    else if (type == 1)
        return_buffer = get_data(key);
    else if (type == 3)
        return_buffer = del_data(key);
    else if (type == 4)
    {
        disply_cache_data();
        return_buffer = "Success";
    }
    else if(type==5)
    {
        show();
        return_buffer="Success";
    }
    else
    {
        return 0;
    }
    // printf("------------------------------------------------------------------------------------\n");
    // printf("Response: %s %ld to the client %d\n", return_buffer, strlen(return_buffer), client_fd);
    send(client_fd, return_buffer, 514, 0);
    return 0;
}
char *put_data(char *key_main, char *val_main)
{
    int x = strlen(key_main);
    char key[x];
    strcpy(key, key_main);
    int y = strlen(key_main);
    char val[y];
    strcpy(val, val_main);
    int a = update_node_cache(key, val);
    char *packet = (char *)malloc(514 * sizeof(char));
    if (a == 0)
        cache_lfu_insert(key, val);
    store_data dt;
    strcpy(dt.key, key);
    strcpy(dt.value, val);
   // printf("Sending for Put with %s %s\n",dt.key,dt.value);
    pthread_t td;
    pthread_create(&td, NULL, put_store, &dt);
   // pthread_detach(td);
    pthread_join(td, NULL);
    strcat(packet, "200");
    int l = (513 - strlen(packet));
    for (int i = 0; i < l; i++)
    {
        strcat(packet, "*");
    }
    packet[513] = '\0';
    return packet;
}
char *get_data(char *key_main)
{
    int x = strlen(key_main);
    char key[x];
    strcpy(key, key_main);
    char *s = NULL,*h=NULL;
    char *packet = (char *)malloc(514 * sizeof(char));
    s = get_node_cache(key);
    if (s != NULL)
    {
       // printf("Foud in the Cache\n");
        strcat(packet, "200");
        strcat(packet, s);
    }
    else
    {
     //   printf("Not Found in the Cache\n");
        h = get_store(key);
     //   printf("Returned from store\n");
        if (h != NULL)
        {
          //  printf("Foud in the Store\n");
            cache_lfu_insert(key, s);
            strcat(packet, "200");
            strcat(packet, s);
        }
        else
        {
           // printf("Not Found in the DB\n");
            strcat(packet, "240 KEY NOT FOUND");
        }
    }
    int l = (513 - strlen(packet));
    for (int i = 0; i < l; i++)
    {
        strcat(packet, "*");
    }

    packet[513] = '\0';
    return packet;
}
char *del_data(char *key_main)
{
    //1. Cache Deletion
    int x = strlen(key_main);
    char key[x];
    strcpy(key, key_main);
    int d;
    delete_node_cache(key);
    d = del_store(key);
    char *packet = (char *)malloc(514 * sizeof(char));
    //2. Store Deletion
    if (d == 1)
        strcat(packet, "200");
    else
        strcat(packet, "240 KEY NOT FOUND");
    int l = (513 - strlen(packet));

    for (int i = 0; i < l; i++)
    {
        strcat(packet, "*");
    }
    packet[513] = '\0';
    return packet;
}

void siginthandler(int sig)
{
    close(server_sock_fd);
    printf("\n--------------------------------------------\n");
    printf("No. Of Active Connections: %d\n", no_of_active_connections);
    printf("SERVER TERMINATED SUCCESSFULLY\n");
    exit(1);
}
void read_conf_file(char *path)
{
    FILE *fptr;
    char s[10];
    if ((fptr = fopen(path, "r")) == NULL)
    {
        printf("Error! opening file");
        // Program exits if file pointer returns NULL.
        exit(1);
    }
    // reads text until newline is encountered
    fgets(PORT, sizeof(PORT), fptr);
    strtok(PORT, "\n");
    //printf("1: %s with lenfth %ld\n", PORT, strlen(PORT));
    fgets(s, sizeof(s), fptr);
    //printf("2: %s", s);
    CLIENTS_PER_THREAD = atoi(s);
    fgets(s, sizeof(s), fptr);
    //printf("3: %s", s);
    CACHE_SIZE = atoi(s);
    fgets(CACHE_REPLACEMENT, sizeof(CACHE_REPLACEMENT), fptr);
    strtok(CACHE_REPLACEMENT, "\n");
    //printf("4: %s with length %ld\n", CACHE_REPLACEMENT, strlen(CACHE_REPLACEMENT));
    fgets(s, sizeof(s), fptr);
    //printf("5: %s", s);
    THREAD_POOL_SIZE_INITIAL = atoi(s);
    fgets(s, sizeof(s), fptr);
    //printf("6: %s", s);
    THREAD_POOL_GROWTH = atoi(s);
    //printf("\nInformation from the conf file port number %s, clients per thread %i, cache size %i, cache replacement %s, THREAD_POOL_SIZE_INITIAL %d and THREAD_POOL_GROWTH %d\n", PORT, CLIENTS_PER_THREAD, CACHE_SIZE, CACHE_REPLACEMENT, THREAD_POOL_SIZE_INITIAL, THREAD_POOL_GROWTH);
    fclose(fptr);
}
void init_threadpool()
{
    thread_pool_head = create_node(0);
    thread_data *last = thread_pool_head;
    for (int i = 1; i < THREAD_POOL_SIZE_INITIAL; i++)
    {
        thread_data *temp = create_node(i);
        last->next = temp;
        last = temp;
    }
    thread_pool_tail = last;
}
thread_data *get_ds(int tid)
{

    thread_data *temp = thread_pool_head;
    while (temp != NULL)
    {
        if (temp->thread_id == tid)
        {
            // printf("Found %p %d\n", temp, temp->thread_id);
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}
void show_data()
{

    thread_data *temp = thread_pool_head;
    while (temp != NULL)
    {
        printf("<--temp=%p with id= %d  and no of connections=%d -->\n", temp, temp->thread_id, temp->no_of_connections);
        temp = temp->next;
    }
}
thread_data *create_node(int index)
{

    thread_data *temp = (thread_data *)malloc(sizeof(thread_data));
    pthread_cond_init(&temp->thread_cond_var, NULL);
    temp->no_of_connections = 0;
    temp->thread_id = index;
    temp->client_fd = -1;
    temp->next = NULL;
    return temp;
}
void extend_threadpool()
{
    thread_data *last = thread_pool_tail;
    for (int i = NO_OF_THREADS; i < (THREAD_POOL_GROWTH + NO_OF_THREADS); i++)
    {
        thread_data *temp = create_node(i);
        last->next = temp;
        last = temp;
    }
    thread_pool_tail = last;
}
