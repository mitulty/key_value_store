#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "lfucache.h"
pthread_mutex_t cache_lock_lfu = PTHREAD_MUTEX_INITIALIZER;
cache_node *cache_head = NULL;
cache_node *create_cache_node(int index)
{
    cache_node *temp = (cache_node *)malloc(sizeof(cache_node));
    temp->next = NULL;
    strcpy(temp->key,"");
    strcpy(temp->value,"");
    pthread_mutex_init(&temp->lock, NULL);
    pthread_cond_init(&temp->reader_can_enter, NULL);
    pthread_cond_init(&temp->writer_can_enter, NULL);
    temp->no_of_readers = 0;
    temp->no_of_writers = 0;
    temp->valid = 0;
    temp->index = index;
    temp->no_of_times_accessed = 0;
    return (temp);
}
void create_cache(int num)
{
    cache_head = create_cache_node(0);
    cache_node *last = cache_head;
    for (int i = 1; i < num; i++)
    {
        cache_node *temp = create_cache_node(i);
        last->next = temp;
        last = temp;
    }
}
int update_node_cache(char *key, char *value)
{
    cache_node *current = cache_head;
    while (current != NULL)
    {
        if ((strcmp(current->key, key) == 0) && current->valid == 1)
        {
            pthread_mutex_lock(&current->lock);
            while (current->no_of_writers != 0 || current->no_of_readers != 0)
                pthread_cond_wait(&current->writer_can_enter, &current->lock);
            current->no_of_writers++;
            pthread_mutex_unlock(&current->lock);
            if (current->valid != 1)
            {
                return 0;
            }
            strcpy(current->value, value);
            current->no_of_times_accessed = 1;
            current->valid = 1;
            pthread_mutex_lock(&current->lock);
            current->no_of_writers--;
            pthread_cond_signal(&current->writer_can_enter);
            pthread_cond_broadcast(&current->reader_can_enter);
            pthread_mutex_unlock(&current->lock);
            return 1;
        }
        current = current->next;
    }
    return 0;
}
void cache_lfu_insert(char *key, char *value)
{
    pthread_mutex_lock(&cache_lock_lfu);
    cache_node *current = cache_head, *lfu = cache_head;
    while (current != NULL)
    {
        if (current->valid == 0)
        {
            lfu = current;
            break;
        }
        if (current->no_of_times_accessed <= lfu->no_of_times_accessed)
            lfu = current;
        current = current->next;
    }
    pthread_mutex_lock(&lfu->lock);
    while (lfu->no_of_writers != 0 || lfu->no_of_readers != 0)
        pthread_cond_wait(&lfu->writer_can_enter, &lfu->lock);
    lfu->no_of_writers++;
    pthread_mutex_unlock(&lfu->lock);
    strcpy(lfu->value, value);
    strcpy(lfu->key, key);
    lfu->no_of_times_accessed = 1;
    lfu->valid = 1;
    pthread_mutex_lock(&lfu->lock);
    lfu->no_of_writers--;
    pthread_cond_signal(&lfu->writer_can_enter);
    pthread_cond_broadcast(&lfu->reader_can_enter);
    pthread_mutex_unlock(&lfu->lock);
    pthread_mutex_unlock(&cache_lock_lfu);
    return;
}
char *get_node_cache(char *key)
{
   // printf("Entered in the Cache Get\n");
    cache_node *current = cache_head;
    while (current != NULL)
    {
        // printf("\n------------------------------------------------------------------------------------\n");
        // printf("key:%s, Value: %s, Valid: %d and Access: %d\n", current->key, current->value, current->valid, current->no_of_times_accessed);
        // printf("\n------------------------------------------------------------------------------------\n");
        if ((strcmp(current->key, key) == 0) && current->valid == 1)
        {
            pthread_mutex_lock(&current->lock);
            while (current->no_of_writers != 0)
                pthread_cond_wait(&current->reader_can_enter, &current->lock);
            current->no_of_readers++;
            pthread_mutex_unlock(&current->lock);
            char *str = (char *)malloc(257 * sizeof(char));
            strcat(str, current->value);
            for (int i = 0; i < 256 - strlen(current->value); i++)
            {
                strcat(str, "*");
            }
            str[256] = '\0';
            current->no_of_times_accessed++;
            pthread_mutex_lock(&current->lock);
            current->no_of_readers--;
            if (current->no_of_readers == 0)
                pthread_cond_signal(&current->writer_can_enter);
            pthread_mutex_unlock(&current->lock);
            return str;
        }
        current = current->next;
    }
    return NULL;
}
int delete_node_cache(char *key)
{
    cache_node *current = cache_head;
    while (current != NULL)
    {
        if ((strcmp(current->key, key) == 0) && current->valid == 1)
        {
            pthread_mutex_lock(&current->lock);
            while (current->no_of_writers != 0 || current->no_of_readers != 0)
                pthread_cond_wait(&current->writer_can_enter, &current->lock);
            current->no_of_writers++;
            pthread_mutex_unlock(&current->lock);
            current->valid = 0;
            current->no_of_times_accessed = 0;
            pthread_mutex_lock(&current->lock);
            current->no_of_writers--;
            pthread_cond_signal(&current->writer_can_enter);
            pthread_cond_broadcast(&current->reader_can_enter);
            pthread_mutex_unlock(&current->lock);
            return 1;
        }
        current = current->next;
    }
    return 0;
}
void disply_cache_data()
{
    printf("\n------------------------------------------------------------------------------------\n");
    cache_node *current = cache_head;
    while (current != NULL)
    {
        printf("Index:%d, key:%s, Value: %s, Valid: %d and Access: %d\n", current->index, current->key, current->value, current->valid, current->no_of_times_accessed);
        current = current->next;
    }
    printf("------------------------------------------------------------------------------------\n");
}
