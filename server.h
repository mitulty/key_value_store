#include <pthread.h>
typedef struct data
{
    char key[257];
    char value[257];
} store_data;

typedef struct thread_node
{
    pthread_cond_t thread_cond_var;
    int no_of_connections;
    int thread_id;
    int client_fd;
    struct thread_node *next;
} thread_data;
thread_data *find_free_thread();
thread_data *get_ds(int tid);

char *put_data(char *key_main, char *val_main);
char *get_data(char *key_main);
char *del_data(char *key_main);
void read_conf_file(char *path);
void *worker_thr(void *args);
void *increase_thread_pool(void *args);
void init_threadpool();
int client_request(int client_fd);
void siginthandler(int sig);
thread_data *create_node(int index);
void extend_threadpool();


