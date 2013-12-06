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
#define INFO_SCRIPT_PATH "script/"
#define INFO_PIPE_FILE "./info_pipe"
#else
#define SERVER_ADDRESS "172.30.0.104"
#define INFO_SCRIPT_PATH "/usr/bin/"
#define INFO_PIPE_FILE "/tmp/info_pipe"
#endif
#define SERVER_PORT "8888"

typedef void (*cb_update)(void);

typedef enum cp_window_type {
    CP_CHAT_WIN = 0,
    CP_SHOW_WIN,
    CP_INFO_WIN,
    CP_LO_INFO_WIN,
    CP_ULIST_WIN,
    CP_MAX_WIN,
}window_type;

typedef enum cp_option_type {
    CP_OPT_HELP = 0,
    CP_OPT_CONNECT,
    CP_OPT_DISCONNECT,
    CP_OPT_SCRIPT,
    CP_OPT_CLEAR,
    CP_OPT_EXIT,
    CP_OPT_MAX,
}option_type;

struct cp_chat_options {
    option_type op_type;
    char *op_name;
    int op_len;
    char *op_desc;
};

struct win_ui {
    int lines;
    int cols;
    int start_x;
    int start_y;
    char left;
    char right;
    char top;
    char bottom;
    char ltop;
    char rtop;
    char lbottom;
    char rbottom;
};

struct cp_win_manage {
    WINDOW *win;
    struct win_ui ui;
    cb_update update_handler;
};

struct msg_list_node {
    struct list_head list;
    int type;
    char time[TIME_BUFFER_SIZE];
    char id[ID_SIZE];
    char message[MESSAGE_BUFFER_SIZE];
};

struct info_list_node {
    struct list_head list;
    int attrs;
    char message[TOTAL_MESSAGE_SIZE];
};

struct usr_list_node {
    struct list_head list;
    int attrs;
    char id[ID_SIZE];
};

void insert_info_list(char *info, int attrs);
void clear_info_list();
void update_info_win();

void update_local_info_win();

void insert_msg_list(int msg_type, char *usr_id, char *msg);
void clear_msg_list();
void update_show_win();

void insert_usr_list(char *id, int attrs);
void delete_usr_list(char *id);
void clear_usr_list();
void update_usr_win();

void update_chat_win();

void resize_win_ui(WINDOW *win, struct win_ui ui, cb_update update);
void draw_win_ui(WINDOW *win, struct win_ui ui);

WINDOW *create_window(struct win_ui ui);
void destroy_window(WINDOW *win);
void *rcv_thread(void *data);
void *info_win_thread(void *data);
void *local_info_win_thread(void *data);
void print_error(char* err_msg);
int connect_server();
void set_env();
void current_time();
void resize_handler(int sig);
void update_win_ui();
void reg_update_win_func();
void first_scr_ui();
void cp_init_chat();
void cp_create_win();
