#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define SA  struct sockaddr
#define EPOLL_SIZE      20
#define MESSAGE_BUFFER_SIZE 256
#define ID_SIZE 50

enum {
    MSG_ALAM_STATE = 0,
    MSG_DATA_STATE,
    MSG_NEWUSER_STATE,
    MSG_DELUSER_STATE,
    MSG_ENDUSER_STATE
};

typedef struct user_data {
    int sock;
    char id[ID_SIZE];
} ud;

typedef struct user_node {
    ud usr_data;
    struct user_node *prev;
    struct user_node *next;
}unode;

typedef unode* p_unode;

typedef struct user_list {
    int count;
    p_unode head;
    p_unode tail;
}ulist;

typedef struct message_st {
    unsigned int state;
    char id[ID_SIZE];
    char message[MESSAGE_BUFFER_SIZE];
}msgst;

void init_ulist(ulist *lptr);
void insert_ulist(ulist *lptr, ud data);
void delete_ulist(ulist *lptr, int key);
