#ifndef __CP_SERVER_H__
#define __CP_SERVER_H__

#include <cp-list.h>
#include <cp-log.h>
#include <cp-common.h>

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

#ifdef TEST
#define SERVER_PORT "8889"
#else
#define SERVER_PORT "8888"
#endif
#define SA  struct sockaddr
#define EPOLL_SIZE 20
#define USER_HASH_SIZE 20

typedef struct user_data {
    int sock;
    char id[ID_SIZE];
} ud;

struct user_list_node {
    struct list_head list;
    ud data;
};

unsigned int hash_func(char *s);
void init_usr_list();
void insert_usr_list(ud data);
void delete_usr_list(ud data);
int exist_usr_list(ud data);
#endif
