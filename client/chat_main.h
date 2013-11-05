#include <ncurses.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <list.h>

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

typedef struct message_st {
    unsigned int state;
    char id[ID_SIZE];
    char message[MESSAGE_BUFFER_SIZE];
}msgst;

struct msg_list_node {
    struct list_head list;
    char message[MESSAGE_BUFFER_SIZE];
};

struct usr_list_node {
    struct list_head list;
    char id[ID_SIZE];
};

void insert_msg_list(char *msg);
void clear_msg_list();
void update_msg_win();

void insert_usr_list(char *id);
void delete_usr_list(char *id);
void clear_usr_list();
void update_usr_win();

WINDOW *create_window(int h, int w, int y, int x);
void destroy_window(WINDOW *win);
void *rcv_thread(void *data);
void print_error(char* err_msg);
int connect_server();
void set_env();
