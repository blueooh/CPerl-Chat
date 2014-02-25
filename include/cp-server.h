#ifndef __CP_SERVER_H__
#define __CP_SERVER_H__

#include <cp-version.h>
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
#include <errno.h>
#include <sys/signal.h>

#ifdef TEST
#define SERVER_PORT "8888"
#else
#define SERVER_PORT "8888"
#endif
#define SA  struct sockaddr
#define EPOLL_SIZE 20
#define CP_PID_FILE "/var/run/cperl-chatd.pid"
#define CP_LOG_FILE "/var/log/cperl-chatd.log"

typedef struct user_data {
    int sock;
    char id[ID_SIZE];
} ud;

struct user_list_node {
    struct list_head list;
    ud data;
};

void init_usr_list();
int create_listen_socket();
int init_epoll();
struct user_list_node *insert_usr_list(ud data);
void delete_usr_list(ud data);
struct user_list_node *exist_usr_list(ud data);
int new_connect_proc(int sock, msgst *packet);
int get_all_user_list(char *buff, int size);
int close_user(int fd);
int reconnect_proc(int sock, msgst *packet);
int cp_unicast_message(int sock, int state, char *data, ...);
int cp_broadcast_message(msgst *packet);
void cp_server_main_loop();
int cp_accept();
int cp_read_user_data(int fd);
int cp_version_compare(const char *cli_ver, const char *srv_ver);
int cp_write_pid();
pid_t cp_read_pid();
void cp_daemon_stop();
#endif
