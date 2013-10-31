#include <ncurses.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SERVER_ADDRESS "172.30.0.104"
#define SERVER_PORT "8888"
#define MESSAGE_BUFFER_SIZE 256
#define ID_SIZE 50

enum {
    MSG_ALAM_STATE = 0,
    MSG_DATA_STATE,
    MSG_NEWUSER_STATE,
    MSG_DELUSER_STATE,
    MSG_ENDUSER_STATE,
};

enum {
    USER_LOGOUT_STATE = 0,
    USER_LOGIN_STATE
};

typedef struct msg_node {
    char msg[MESSAGE_BUFFER_SIZE];
    struct msg_node *prev;
    struct msg_node *next;
}mnode;

typedef mnode* p_mnode;

typedef struct msg_list {
    int count;
    p_mnode head;
    p_mnode tail;
}mlist;

typedef struct user_node {
    char id[ID_SIZE];
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

WINDOW *create_window(int h, int w, int y, int x);
void destroy_window(WINDOW *win);
void update_ulist_win(ulist *list);
void init_ulist(ulist *lptr);
void insert_ulist(ulist *lptr, char *id);
void clear_ulist(ulist *lptr);
void update_show_win(mlist *list);
void init_mlist(mlist *lptr);
void insert_mlist(mlist *lptr, char *msg);
void clear_mlist(mlist *lptr);
void *rcv_thread(void *data);
void print_error(char* err_msg);
int connect_server();
void disconnect_server();
void delete_ulist(ulist *lptr, char *key);
void set_env();
