#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "kvstore.h"
#include "server.h"

typedef struct meta_node
{
    struct meta_node *next;
    char key[257];
    int index;
    int valid;

    //synochronisation attributes
    pthread_cond_t reader_can_enter, writer_can_enter;
    pthread_mutex_t lock;
    int no_of_readers;
    int no_of_writers;

} meta_data;

meta_data *head = NULL;
char *dirname;
int num_of_nodes = 0;
pthread_mutex_t search_meta_data = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t get_store_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t last_node_insert = PTHREAD_MUTEX_INITIALIZER;

void show()
{
    printf("\n---------------------------------------------------------------------\n");
    meta_data *ptr = head;
    while (ptr != NULL)
    {
        printf("<index %d  key: %s valid: %d >\n", ptr->index, ptr->key, ptr->valid);
        ptr = ptr->next;
    }
    printf("\n---------------------------------------------------------------------\n");
}
char *get_filename(int f)
{
    char *filename = (char *)malloc(sizeof(char) * 256);
    char *filename_with_path = (char *)malloc(sizeof(char) * 260);

    sprintf(filename, "%d", f);
    strcat(filename, ".txt");
    sprintf(filename_with_path, "%s/", dirname);
    strcat(filename_with_path, filename);

    return (filename_with_path);
}

void save_file(int index, char *value)
{
    char *filename = get_filename(index);
    // printf("filename is %s \n", filename);
    FILE *fp;
    fp = fopen(filename, "w");

    if (fp == NULL)
        exit(EXIT_FAILURE);

    fprintf(fp, "%s", value);

    fclose(fp);
}

int search_store(char *key)
{

    pthread_mutex_lock(&search_meta_data);
    if (head != NULL)
    {
        meta_data *ptr = head;
        while (ptr != NULL)
        {
            if (strcmp(ptr->key, key) == 0 && ptr->valid == 1)
            {
                pthread_mutex_unlock(&search_meta_data);
                return (ptr->index); //found a particular node
            }

            ptr = ptr->next;
        }
        pthread_mutex_unlock(&search_meta_data);
        return (-1);
    }
    pthread_mutex_unlock(&search_meta_data);
    return (-2);
}

char *get_store(char *key)
{
    int index = search_store(key);
    if (index == -1 || index == -2)
    {
        return (NULL);
    }
    else
    { /////////////////////// reader lock wait only if writers are present /////////////////////////////

        pthread_mutex_lock(&get_store_lock);
        char *filename = get_filename(index);
        FILE *fp;
        char *value = malloc(sizeof(char) * 257);
        size_t len = 0;
        fp = fopen(filename, "r");
        if (fp == NULL)
            exit(EXIT_FAILURE);

        getline(&value, &len, fp);
        fclose(fp);
        pthread_mutex_unlock(&get_store_lock);
        return (value);
    }
}

void *put_store(void *data)
{
    store_data d;
    d = *((store_data *)(data));
    int index = search_store(d.key);
    // printf("Call for Put with  %s %s\n",d.key,d.value);
    // if(head!=NULL)
    // printf("HEAD is %s %d\n",head->key,head->index);
    meta_data *ptr = head;
    //-2 means head was null and -1 means did not found >=0 means found
    if (index == -2)
    {

        head = (meta_data *)malloc(sizeof(meta_data));
        head->index = num_of_nodes;
        head->valid = 1;
        strcpy(head->key, d.key);
        head->next = NULL;
        num_of_nodes++;
        //initialization
        head->no_of_readers = 0;
        head->no_of_writers = 0;
        pthread_mutex_init(&head->lock, NULL);
        pthread_cond_init(&head->reader_can_enter, NULL);
        pthread_cond_init(&head->writer_can_enter, NULL);
        save_file(head->index, d.value);
        return NULL;
    }
    else if (index >= 0)
    {

        while (ptr != NULL)
        {

            if (ptr->index == index)
            { /////////////////////wait till all readers and writer have exited//////////////////////////
                pthread_mutex_lock(&ptr->lock);
                while (ptr->no_of_writers != 0 || ptr->no_of_readers != 0)
                    pthread_cond_wait(&ptr->writer_can_enter, &ptr->lock);
                ptr->no_of_writers++;
                pthread_mutex_unlock(&ptr->lock);

                ptr->valid = 1;
                strcpy(ptr->key, d.key);

                save_file(ptr->index, d.value);

                //////////// signal unlock writter-- etc//////////
                pthread_mutex_lock(&ptr->lock);
                ptr->no_of_writers--;
                pthread_cond_signal(&ptr->writer_can_enter);
                pthread_cond_broadcast(&ptr->reader_can_enter);
                pthread_mutex_unlock(&ptr->lock);

                return NULL;
            }
            ptr = ptr->next;
        }
    }
    else if (index == -1)
    {

        //list do not conatins any free space,or till empty node i.e valid = 0
        while (ptr->next != NULL)
        {
            if (ptr->valid == 0)
                break;       //traversing till valid
            ptr = ptr->next; // traversing till end
        }

        if (ptr->valid == 0)
        { // when empty node found
            /////////////////////wait till all readers and writer have exited//////////////////////////
            pthread_mutex_lock(&ptr->lock);
            while (ptr->no_of_writers != 0 || ptr->no_of_readers != 0)
                pthread_cond_wait(&ptr->writer_can_enter, &ptr->lock);
            ptr->no_of_writers++;
            pthread_mutex_unlock(&ptr->lock);

            strcpy(ptr->key, strdup(d.key));
            ptr->valid = 1;
            save_file(ptr->index, d.value);

            //////////// signal unlock writter-- etc///////////
            pthread_mutex_lock(&ptr->lock);
            ptr->no_of_writers--;
            pthread_cond_signal(&ptr->writer_can_enter);
            pthread_cond_broadcast(&ptr->reader_can_enter);
            pthread_mutex_unlock(&ptr->lock);

            return NULL;
        }
        ////////////////////inserting at last should be sequential////waiting for inserting at last of the list////

        /////////////////////wait till all readers and writer have exited//////////////////////////
        pthread_mutex_lock(&last_node_insert);

        ptr = head;
        while (ptr->next != NULL)
        {
            ptr = ptr->next; // traversing till end
        }

        meta_data *temp;
        temp = (meta_data *)malloc(sizeof(meta_data));
        temp->next = NULL;
        strcpy(temp->key, strdup(d.key));
        temp->valid = 1;
        temp->index = num_of_nodes;
        num_of_nodes++;
        temp->no_of_readers = 0;
        temp->no_of_writers = 0;
        pthread_mutex_init(&temp->lock, NULL);
        pthread_cond_init(&temp->reader_can_enter, NULL);
        pthread_cond_init(&temp->writer_can_enter, NULL);
        save_file(temp->index, d.value);
        ptr->next = temp;

        pthread_mutex_unlock(&last_node_insert);

        return NULL;
    }
    return NULL;
}

int del_store(char *key)
{ // return 0 if not present and 1 if present
    //if already present then delete else say not present
    if (head != NULL)
    {
        meta_data *ptr = head;
        while (ptr != NULL)
        {
            if (strcmp(ptr->key, key) == 0 && ptr->valid != 0)
            { /////////////////////wait till all readers and writer have exited//////////////////////////
                pthread_mutex_lock(&ptr->lock);
                while (ptr->no_of_writers != 0 || ptr->no_of_readers != 0)
                    pthread_cond_wait(&ptr->writer_can_enter, &ptr->lock);
                ptr->no_of_writers++;
                pthread_mutex_unlock(&ptr->lock);

                ptr->valid = 0;

                //////////// signal unlock writter-- etc//////////
                pthread_mutex_lock(&ptr->lock);
                ptr->no_of_writers--;
                pthread_cond_signal(&ptr->writer_can_enter);
                pthread_cond_broadcast(&ptr->reader_can_enter);
                pthread_mutex_unlock(&ptr->lock);
                return (1);
            }
            ptr = ptr->next;
        }
    }
    return (0);
}
void init_store()
{

    // int check;
    dirname = "db";
    // clrscr();
    mkdir(dirname, 0777);
}
