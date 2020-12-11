#include<pthread.h>
typedef struct cache_node
{
    struct cache_node *next;
    char key[256];
    char value[256];
    pthread_mutex_t lock;
    pthread_cond_t reader_can_enter, writer_can_enter;
    int no_of_readers;
    int no_of_writers;
    int no_of_times_accessed;
    int valid;
    int index;// For Debugging Only
} cache_node;

int update_node_cache(char *key, char *value);
char *get_node_cache(char *key);
int delete_node_cache(char *key);
void create_cache(int num);
void cache_lfu_insert(char *key, char *value);
void disply_cache_data();
cache_node *create_cache_node(int index);