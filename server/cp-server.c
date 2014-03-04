#include <cp-server.h>

struct list_head usr_list[USER_HASH_SIZE];
int log_fd;
int efd;
int listen_sock;
int clilen;
struct epoll_event *events;

int main(int argc, char *argv[])
{
    if(cp_init_log(CP_LOG_FILE) < 0) {
        printf("init log error\n");
        return -1;
    }

    if(argc >= 2) {
        if(!strcmp(argv[1], "stop")) {
            cp_log("daemon stopping...");
            cp_daemon_stop();
            return 0;

        } else if(!strcmp(argv[1], "daemon")) {
            if(daemon(1, 1)) {
                cp_log("failed daemonize");
                return -1;
            }
            if(cp_write_pid() < 0) {
                return -1;
            }
        }
    }

    cp_log("start cperl-chat...(v.%s)", cp_version);

    clilen = sizeof(struct sockaddr_in);

    init_usr_list();

    if(create_listen_socket() < 0) {
        printf("failed listen socket\n");
        return -1;
    }

    if(init_epoll() < 0) {
        printf("init epoll error\n");
        return -1;
    }

    cp_server_main_loop();

    return 0;
}

void init_usr_list()
{
    int i;

    for(i = 0; i < USER_HASH_SIZE; i++) {
        INIT_LIST_HEAD(&usr_list[i]);
    }
}

struct user_list_node *insert_usr_list(ud data)
{
    struct user_list_node *node;
    unsigned int hash;

    node = (struct user_list_node *)malloc(sizeof(struct user_list_node));
    node->data.sock = data.sock;
    strcpy(node->data.id, data.id);
    hash = hash_func(data.id);
    list_add(&node->list, &usr_list[hash]);
    cp_log("insert user list: id(%s), sock(%d), hash(%d)", data.id, data.sock, hash);
}

void delete_usr_list(ud data)
{
    struct user_list_node *node, *tnode;
    unsigned int hash;

    hash = hash_func(data.id);
    list_for_each_entry_safe(node, tnode, &usr_list[hash], list) {
        if(!strcmp(data.id, node->data.id)) {
            list_del(&node->list);
            free(node);
            cp_log("delete user list: id(%s), sock(%d), hash(%d)", data.id, data.sock, hash);
        }
    }
}

struct user_list_node *exist_usr_list(ud data)
{
    struct user_list_node *node = NULL;
    unsigned int hash;

    hash = hash_func(data.id);
    list_for_each_entry(node, &usr_list[hash], list) {
        if(!strcmp(data.id, node->data.id)) {
            return node;
        }
    }

    return NULL;
}

int new_connect_proc(int sock, CP_PACKET *p)
{
    struct user_list_node *node;
    int hash, idx = 0;
    ud data;
    CP_PACKET noti_packet;
    char usr_list_buf[MESSAGE_BUFFER_SIZE];

    if(cp_version_compare(p->cp_h.version, cp_version) < 0) {
        cp_unicast_message(sock, MSG_ALAM_STATE, 
                "version compare fail, update your cperl-chat version == %s", cp_version);
        remove_user(sock);
        return -1;
    }

    data.sock = sock;
    strcpy(data.id, p->cp_h.id);
    insert_usr_list(data);

    get_all_user_list(usr_list_buf, sizeof(usr_list_buf));
    cp_unicast_message(sock, MSG_USERLIST_STATE, usr_list_buf);

    noti_packet.cp_h.state = MSG_NEWUSER_STATE;
    strcpy(noti_packet.cp_h.id, p->cp_h.id);
    cp_broadcast_message(&noti_packet);

    return 0;
}

int get_all_user_list(char *buff, int size)
{
    int hash, idx = 0;
    struct user_list_node *node;

    for(hash = 0; hash < USER_HASH_SIZE; hash++) {
        list_for_each_entry(node, &usr_list[hash], list) {
            idx += snprintf(buff + idx, size - idx, "%s%s", node->data.id, USER_DELIM);
        }
    }

    return idx;
}

int remove_user(int fd)
{
    int hash, found = 0;
    struct user_list_node *node;
    ud user_data;
    CP_PACKET packet;

    /* find user in hash table weather or not exists */
    for(hash = 0; hash < USER_HASH_SIZE; hash++) {
        list_for_each_entry(node, &usr_list[hash], list) {
            if(node->data.sock == fd) {
                found = 1;
                strcpy(user_data.id, node->data.id);
                user_data.sock = node->data.sock;
                break;
            }
        }
    }

    if(found) {
        cp_log("found close user: id(%s), sock(%d)", user_data.id, user_data.sock);
        delete_usr_list(user_data);
        packet.cp_h.state = MSG_DELUSER_STATE;
        strcpy(packet.cp_h.id, user_data.id);
        cp_broadcast_message(&packet);

    } else {
        cp_log("cannot fond user, anyway force close: sock(%d)", fd);
    }

    epoll_ctl(efd, EPOLL_CTL_DEL, user_data.sock, events);

    return close(fd);
}

int reconnect_proc(int sock, CP_PACKET *p)
{
    int ret = 0;
    struct user_list_node *node;
    CP_PACKET noti_packet;
    char usr_list_buff[MESSAGE_BUFFER_SIZE];
    ud user_data;

    user_data.sock = sock;
    strcpy(user_data.id, p->cp_h.id);

    if(node = exist_usr_list(user_data)) {
        /* if user exists in hash table, update sock fd */
        epoll_ctl(efd, EPOLL_CTL_DEL, node->data.sock, events);
        close(node->data.sock);
        node->data.sock = user_data.sock;
        cp_log("user socket changed: user-id(%s), sock(%d)", node->data.id, node->data.sock);

        noti_packet.cp_h.state = MSG_USERLIST_STATE;
        get_all_user_list(usr_list_buff, sizeof(usr_list_buff));
        cp_unicast_message(sock, MSG_USERLIST_STATE, usr_list_buff);

    } else {
        cp_log("user try re-connect..., not exists so new add: user-id(%s), sock(%d)", user_data.id, user_data.sock);
        /* If user not exsits in hash table, process new connection */
        new_connect_proc(sock, p);
    }

    return ret;
}

int cp_unicast_message(int sock, int state, char *data, ...)
{
    int len = -1;
    CP_PACKET packet;

    cp_va_format(data);

    if(vbuffer) {
        strcpy(packet.cp_h.version, cp_version);
        packet.cp_h.state = state;
        strcpy(packet.message, vbuffer);

        if(len = write(sock, (char *)&packet, sizeof(packet)) < 0) {
            cp_log("unicast socket error: sock(%d), errno(%d), strerror(%s)", 
                    sock, errno, strerror(errno));
            return -1;
        }
    }

    return len;
}

int cp_broadcast_message(CP_PACKET *p)
{
    int hash, len = 0;
    struct user_list_node *node;

    for(hash = 0; hash < USER_HASH_SIZE; hash++) {
        list_for_each_entry(node, &usr_list[hash], list) {
            if(len = write(node->data.sock, (char *)p, sizeof(CP_PACKET)) < 0) {
                cp_log("broadcast socket error: user(%s), sock(%d), errno(%d), strerror(%s)", 
                        node->data.id, node->data.sock, errno, strerror(errno));
            }
        }
    }

    return len;
}

int init_epoll()
{
    struct epoll_event ev;

    /* create events as many as epoll poll size */
    events = (struct epoll_event *)malloc(sizeof(*events) * EPOLL_SIZE);
    if(!events) {
        cp_log("events malloc error");
        return -1;
    }

    /* create epoll discriptor */
    if ((efd = epoll_create(100)) < 0) {
        cp_log("epoll_create error: %s, errno(%d)", strerror(errno), errno);
        return -1;
    }

    /* add listen socket fd */
    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, listen_sock, &ev) < 0) {
        cp_log("listen socket epoll add error: %s, errno(%d)", strerror(errno), errno);
        return -1;
    }

    return 1;
}

int create_listen_socket()
{
    struct sockaddr_in addr;

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);  
    if (listen_sock < 0)
    {
        cp_log("listen socket error : %s, errno(%d)", strerror(errno), errno);
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(SERVER_PORT));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind (listen_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        close(listen_sock);
        cp_log("listen socke bind error:%s, errno(%d)", strerror(errno), errno);
        return -1;
    }

    if(listen(listen_sock, 5) < 0) {
        cp_log("listen error:%s, errno(%d)", strerror(errno), errno);
        return -1;
    }

    return listen_sock;
}

void cp_server_main_loop()
{
    int n, i;

    while(1)
    {
        /* check event in epoll pool */
        n = epoll_wait(efd, events, EPOLL_SIZE, -1);
        if (n < 0 ) {
            cp_log("epoll wait error: %s, errno(%d)", strerror(errno), errno);
            continue;
        }

        for (i = 0; i < n; i++) {
            /* If event socket is listen socket, accept client socket */
            if (events[i].data.fd == listen_sock) {
                cp_accept();

            } else {
                /* read data from client fd */
                cp_read_user_data(events[i].data.fd);
            }
        }
    }
}

int cp_accept()
{
    struct sockaddr_in clientaddr;
    struct epoll_event ev;
    int cfd;

    if((cfd = accept(listen_sock, (SA *)&clientaddr, &clilen)) < 0) {
        cp_log("accetp error: %s, (%d)", strerror(errno), errno);
        return -1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = cfd;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev)) {
        cp_log("client socket add epoll error: %s, (%d)", strerror(errno), errno);
        close(cfd);
        return -1;
    }

    //cp_unicast_message(cfd, MSG_ALAM_STATE, "Welcome to CPerl-Chat World!");

    return 0;
}

int cp_read_user_data(int fd)
{
    int readn;
    CP_PACKET rcv_packet;
    ud user_data;

    readn = read(fd, (char *)&rcv_packet, 1024);
    if (readn <= 0) {
        /* handle close clients otherwise read error */
        cp_log("closed socket connection: sock(%d), readn(%d), errno(%d), strerror(%s)", 
                fd, readn, errno, strerror(errno));
        remove_user(fd);

    } else {
        user_data.sock = fd;
        strcpy(user_data.id, rcv_packet.cp_h.id);

        switch(rcv_packet.cp_h.state) {
            case MSG_RECONNECT_STATE:
                cp_log("user try re-connect..., update socket: user-id(%s), sock(%d)", 
                        rcv_packet.cp_h.id, fd);
                reconnect_proc(fd, &rcv_packet);
                break;

            case MSG_NEWCONNECT_STATE:
                if(exist_usr_list(user_data)) {
                    cp_log("new connect user id exists, force to close...: user-id(%s)", 
                            rcv_packet.cp_h.id);
                    cp_unicast_message(user_data.sock, MSG_ALAM_STATE, 
                            "ID Exists already, re-connect to server after change your ID!");
                    epoll_ctl(efd, EPOLL_CTL_DEL, fd, events);
                    close(fd);
                    break;

                } else {
                    cp_log("log-in user: ver(%s), id(%s), sock(%d)", 
                            rcv_packet.cp_h.version, rcv_packet.cp_h.id, user_data.sock);
                    new_connect_proc(fd, &rcv_packet);
                }
                break;

            case MSG_DATA_STATE:
                /* broadcast packet to all user */
                cp_broadcast_message(&rcv_packet);
                break;

            case MSG_AVAILTEST_STATE:
                break;

            default:
                break;
        }
    }

    return readn;
}

int cp_version_compare(const char *cli_ver, const char *srv_ver)
{
    if(strcmp(cli_ver, srv_ver)) {
        cp_log("version compare fail: cli(%s), srv(%s)", cli_ver, srv_ver);
        return -1;
    }

    return 0;
}

int cp_write_pid()
{
    int fd, len;
    char pid_buff[64], *file = CP_PID_FILE;
    pid_t pid;

    if(!access(file, R_OK)) {
        cp_log("daemon is still running..., firstly kill deamon...");
        return -1;
    }

    if((fd = open(file, O_RDWR | O_CREAT, 0644)) <= 0) {
        cp_log("failed to open pid file...: %s", file);
        return -1;
    }

    pid = getpid();
    len = sprintf(pid_buff, "%u", pid);
    write(fd, pid_buff, len);
    close(fd);

    cp_log("daemon pid is %d", pid);

    return len;
}

pid_t cp_read_pid()
{
    int fd;
    pid_t pid;
    char pid_buff[64], *file = CP_PID_FILE;

    if((fd = open(file, O_RDWR)) < 0) {
        cp_log("failed to open pid...: %s", file);
        return -1;
    }

    if(read(fd, pid_buff, sizeof(pid_buff)) < 0) {
        cp_log("failed to read pid...: %s", file);
        return -1;
    }
    pid = (pid_t)atoi(pid_buff);
    close(fd);

    cp_log("read pid...: %d", pid);

    return pid;
}

void cp_daemon_stop()
{
    pid_t pid;

    if((pid = cp_read_pid()) < 0) {
        cp_log("pid read error");
        return;
    }

    cp_log("kill daemon..: pid(%d)", pid);

    kill(pid, SIGKILL);
    unlink(CP_PID_FILE);

    return;
}
