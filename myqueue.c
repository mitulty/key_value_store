#include<stdio.h>
#include<stdlib.h>
#include "myqueue.h"
struct connection_node
{
    struct connection_node *next;
    int client_socket_fd;
};
typedef struct connection_node conn_node_t;
conn_node_t *queue_head = NULL;
conn_node_t *tail = NULL;
void enqueue(int client_socket_fd)
{
    conn_node_t *temp = (conn_node_t *)malloc(sizeof(conn_node_t));
    temp->client_socket_fd = client_socket_fd;
    temp->next = NULL;
    if (tail == NULL)
    {
        queue_head = temp;
    }
    else
    {
        tail->next = temp;
    }
    tail = temp;
}

int dequeue()
{
    if (queue_head == NULL)
    {
        return -1;
    }
    else
    {
        int r = queue_head->client_socket_fd;
        conn_node_t *temp = queue_head;
        queue_head = queue_head->next;
        if (queue_head == NULL)
        {
            tail = NULL;
        }
        free(temp);
        return r;
    }
}
void show_fifo()
{
    printf("\n------------------------------------------\n");
    conn_node_t *temp = queue_head;
    int r;
    while(temp!=NULL)
    {
        r=temp->client_socket_fd;
        printf("%d ",r);
        temp=temp->next;
    }
    printf("\n------------------------------------------\n");
}
