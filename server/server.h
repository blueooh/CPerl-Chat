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
#include <list.h>

#define SA  struct sockaddr
#define EPOLL_SIZE      20
#define MESSAGE_BUFFER_SIZE 512
#define ID_SIZE 50
#define USER_HASH_SIZE 20

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

typedef struct message_st {
    unsigned int state;
    char id[ID_SIZE];
    char message[MESSAGE_BUFFER_SIZE];
}msgst;

struct user_list_node {
    struct list_head list;
    ud data;
};

unsigned int hash_func(char *s);
void init_usr_list();
void insert_usr_list(ud data);
void delete_usr_list(ud data);
int exist_usr_list(ud data);
