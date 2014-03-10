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

int insert_usr_list(int sock, char *id)
{
    struct user_list_node *node;
    unsigned int hash = sock % USER_HASH_SIZE;

    node = (struct user_list_node *)malloc(sizeof(struct user_list_node));
    if(!node) {
        return -1;
    }

    node->sock = sock;
    strcpy(node->id, id);
    list_add(&node->list, &usr_list[hash]);

    cp_log("insert user list: id(%s), sock(%d), hash(%d)", id, sock, hash);

    return 0;
}

void delete_usr_list(int sock)
{
    struct user_list_node *node, *tnode;
    unsigned int hash = sock % USER_HASH_SIZE;

    list_for_each_entry_safe(node, tnode, &usr_list[hash], list) {
        if(sock == node->sock) {
            cp_log("delete user list: id(%s), sock(%d), hash(%d)", node->id, node->sock, hash);
            list_del(&node->list);
            free(node);
        }
    }
}

struct user_list_node *exist_usr_id(char *id)
{
    struct user_list_node *node = NULL;
    int hash_idx;

    for(hash_idx = 0; hash_idx < USER_HASH_SIZE; hash_idx++) {
        list_for_each_entry(node, &usr_list[hash_idx], list) {
            if(!strcmp(id, node->id)) {
                return node;
            }
        }
    }

    return NULL;
}

int new_connect_proc(int sock, CP_PACKET *p)
{
    struct user_list_node *node;
    int len, hash, idx = 0;
    CP_PACKET noti_packet;
    char usr_list_buf[MESSAGE_BUFFER_SIZE];

    if(cp_version_compare(p->cp_h.version, cp_version) < 0) {
        cp_unicast_message(sock, MSG_ALAM_STATE, 
                "version compare fail, update your cperl-chat version == %s", cp_version);
        remove_user(sock);
        return -1;
    }

    insert_usr_list(sock, p->cp_h.id);

    get_all_user_list(usr_list_buf, sizeof(usr_list_buf));
    len = cp_unicast_message(sock, MSG_USERLIST_STATE, usr_list_buf);
    cp_log("send user list to client : id(%s), user list(%s), packet len(%d)", p->cp_h.id, usr_list_buf, len);

    noti_packet.cp_h.state = MSG_NEWUSER_STATE;
    strcpy(noti_packet.cp_h.version, p->cp_h.version);
    strcpy(noti_packet.cp_h.id, p->cp_h.id);
    noti_packet.cp_h.dlen = 0;
    cp_broadcast_message(&noti_packet);

    return 0;
}

int get_all_user_list(char *buff, int size)
{
    int hash, idx = 0;
    struct user_list_node *node;

    for(hash = 0; hash < USER_HASH_SIZE; hash++) {
        list_for_each_entry(node, &usr_list[hash], list) {
            idx += snprintf(buff + idx, size - idx, "%s%s", node->id, USER_DELIM);
        }
    }

    return idx;
}

int remove_user(int fd)
{
    struct user_list_node *node;
    CP_PACKET packet;

    /* find user in hash table weather or not exists */
    node = get_usr_list(fd);
    if(node) {
        cp_log("found close user: id(%s), sock(%d)", node->id, node->sock);
        delete_usr_list(fd);
        packet.cp_h.state = MSG_DELUSER_STATE;
        strcpy(packet.cp_h.version, cp_version);
        strcpy(packet.cp_h.id, node->id);
        packet.cp_h.dlen = 0;
        cp_broadcast_message(&packet);

    } else {
        cp_log("cannot fond user, anyway force close: sock(%d)", fd);
    }

    epoll_ctl(efd, EPOLL_CTL_DEL, fd, events);

    return close(fd);
}

int reconnect_proc(int sock, CP_PACKET *p)
{
    int ret = 0;
    struct user_list_node *node;
    CP_PACKET noti_packet;
    char usr_list_buff[MESSAGE_BUFFER_SIZE];

    if(node = exist_usr_id(p->cp_h.id)) {
        /* if user exists in hash table, update sock fd */
        epoll_ctl(efd, EPOLL_CTL_DEL, node->sock, events);
        close(sock);
        node->sock = sock;
        cp_log("user socket changed: user-id(%s), sock(%d)", node->id, node->sock);

        noti_packet.cp_h.state = MSG_USERLIST_STATE;
        get_all_user_list(usr_list_buff, sizeof(usr_list_buff));
        cp_unicast_message(sock, MSG_USERLIST_STATE, usr_list_buff);

    } else {
        cp_log("user try re-connect..., not exists so new add: user-id(%s), sock(%d)", p->cp_h.id, sock);
        /* If user not exsits in hash table, process new connection */
        new_connect_proc(sock, p);
    }

    return ret;
}

int cp_unicast_message(int sock, int state, char *data, ...)
{
    int len = -1, vbuffer_len;
    CP_PACKET_HEADER cph;
    char send_buffer[1024];

    strcpy(cph.version, cp_version);
    cph.state = state;
    cp_va_format(data);
    if(vbuffer) {
        cph.dlen = strlen(vbuffer);
    } else {
        cph.dlen = 0;
    }

    memcpy(send_buffer, &cph, sizeof(CP_PACKET_HEADER));
    if(cph.dlen) {
        memcpy(send_buffer + sizeof(CP_PACKET_HEADER), vbuffer, cph.dlen);
    }

    if((len = write(sock, send_buffer, sizeof(CP_PACKET_HEADER) + cph.dlen)) < 0) {
        cp_log("unicast socket error: sock(%d), errno(%d), strerror(%s)", 
                sock, errno, strerror(errno));
        goto out;
    }

out:
    if(vbuffer) {
        free(vbuffer);
    }

    return len;
}

int cp_broadcast_message(CP_PACKET *p)
{
    int hash, len = 0;
    struct user_list_node *node;
    char send_buffer[1024];

    memcpy(send_buffer, &p->cp_h, sizeof(CP_PACKET_HEADER));
    memcpy(send_buffer + sizeof(CP_PACKET_HEADER), p->data, p->cp_h.dlen);

    for(hash = 0; hash < USER_HASH_SIZE; hash++) {
        list_for_each_entry(node, &usr_list[hash], list) {
            if(len = write(node->sock, send_buffer, sizeof(CP_PACKET_HEADER) + p->cp_h.dlen) < 0) {
                cp_log("broadcast socket error: user(%s), sock(%d), errno(%d), strerror(%s)", 
                        node->id, node->sock, errno, strerror(errno));
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

    return 0;
}

int cp_read_user_data(int fd)
{
    int readn;
    char rcv_buffer[1024];
    CP_PACKET packet;

    packet.data = NULL;

    readn = read(fd, rcv_buffer, 1024);
    if (readn <= 0) {
        /* handle close clients otherwise read error */
        cp_log("closed socket connection: sock(%d), readn(%d), errno(%d), strerror(%s)", 
                fd, readn, errno, strerror(errno));
        remove_user(fd);

    } else {
        if(readn < sizeof(CP_PACKET_HEADER)) {
            cp_log("receive packet size small than header size..., read len(%d)\n", readn);
            goto out;
        }

        memcpy(&packet.cp_h, rcv_buffer, sizeof(CP_PACKET_HEADER));
        //cp_log("ver:%s, state:%d, dlen:%d", packet.cp_h.version, packet.cp_h.state, packet.cp_h.dlen);

        if(readn != (sizeof(CP_PACKET_HEADER) + packet.cp_h.dlen)) {
            cp_log("receive packet length strange..., read len(%d)", readn);
            goto out;
        }

        packet.data = (char *)malloc(packet.cp_h.dlen);
        memcpy(packet.data, rcv_buffer + sizeof(CP_PACKET_HEADER), packet.cp_h.dlen);

        switch(packet.cp_h.state) {
            case MSG_RECONNECT_STATE:
                cp_log("user try re-connect..., update socket: user-id(%s), sock(%d)", packet.cp_h.id, fd);
                reconnect_proc(fd, &packet);
                break;

            case MSG_NEWCONNECT_STATE:
                if(exist_usr_id(packet.cp_h.id)) {
                    cp_log("new connect user id exists, force to close...: user-id(%s)", packet.cp_h.id);
                    cp_unicast_message(fd, MSG_ALAM_STATE, 
                            "ID Exists already, re-connect to server after change your ID!");
                    epoll_ctl(efd, EPOLL_CTL_DEL, fd, events);
                    close(fd);
                    break;

                } else {
                    cp_log("log-in user: ver(%s), id(%s), sock(%d)", packet.cp_h.version, packet.cp_h.id, fd);
                    new_connect_proc(fd, &packet);
                }
                break;

            case MSG_DATA_STATE:
                /* broadcast packet to all user */
                cp_broadcast_message(&packet);
                break;

            case MSG_AVAILTEST_STATE:
                break;

            default:
                cp_log("invalid state packet: sock(%d), state(%d)", fd, packet.cp_h.state);
                break;
        }
    }

out:
    if(packet.data) {
        free(packet.data);
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

struct user_list_node *get_usr_list(int sock)
{
    int hash = sock % USER_HASH_SIZE;
    struct user_list_node *node;

    list_for_each_entry(node, &usr_list[hash], list) {
        if(node->sock == sock) {
            return node;
        }
    }

    return NULL;
}
