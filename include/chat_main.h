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
#include <sys/stat.h> 
#include <sys/select.h> 
#include <fcntl.h> 
#include <stdio.h> 
#include <common.h>
#include <signal.h>
#include <sys/ioctl.h>

#ifdef TEST
#define SERVER_ADDRESS "127.0.0.1"
#define INFO_SCRIPT_PATH "../script/"
#else
#define SERVER_ADDRESS "172.30.0.104"
#define INFO_SCRIPT_PATH "/usr/bin/"
#endif
#define SERVER_PORT "8888"

struct cp_win_ui {
    int lines;
    int cols;
    int start_x;
    int start_y;
};

struct msg_list_node {
    struct list_head list;
    char message[TOTAL_MESSAGE_SIZE];
};

struct info_list_node {
    struct list_head list;
    char message[TOTAL_MESSAGE_SIZE];
};

struct usr_list_node {
    struct list_head list;
    char id[ID_SIZE];
};

void insert_info_list(char *info);
void clear_info_list();
void update_info_win();
void insert_msg_list(char *msg);
void clear_msg_list();
void update_msg_win();

void insert_usr_list(char *id);
void delete_usr_list(char *id);
void clear_usr_list();
void update_usr_win();

void update_chat_win();

WINDOW *create_window(int h, int w, int y, int x);
void destroy_window(WINDOW *win);
void *rcv_thread(void *data);
void *info_win_thread(void *data);
void print_error(char* err_msg);
int connect_server();
void set_env();
void current_time();
void resize_handler(int sig);
void update_win_ui();
void init_cp_chat();
