#include <server.h>

struct list_head usr_list[USER_HASH_SIZE];

int main(int argc, char **argv)
{
    struct sockaddr_in addr, clientaddr;
    struct eph_comm *conn;
    int sfd;
    int cfd;
    int clilen;
    int flags = 1;
    int n, i;
    int readn;
    struct epoll_event ev,*events;

    int efd;

    init_usr_list();

    // 이벤트 풀의 크기만큼 events구조체를 생성한다.
    events = (struct epoll_event *)malloc(sizeof(*events) * EPOLL_SIZE);

    // epoll_create를 이용해서 epoll 지정자를 생성한다. 
    if ((efd = epoll_create(100)) < 0)
    {
        perror("epoll_create error");
        return 1;
    }


    // --------------------------------------
    // 듣기 소켓 생성을 위한 일반적인 코드
    clilen = sizeof(clientaddr);
    sfd = socket(AF_INET, SOCK_STREAM, 0);  
    if (sfd == -1)
    {
        perror("socket error :");
        close(sfd);
        return 1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(SERVER_PORT));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind (sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        close(sfd);
        return 1;
    }
    listen(sfd, 5);
    // --------------------------------------

    // 만들어진 듣기 소켓을 epoll이벤트 풀에 추가한다.
    // EPOLLIN(read) 이벤트의 발생을 탐지한다.
    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev);
    while(1)
    {
        // epoll이벤트 풀에서 이벤트가 발생했는지를 검사한다.
        n = epoll_wait(efd, events, EPOLL_SIZE, -1);
        if (n == -1 )
        {
            perror("epoll wait error");
        }

        //printf("event count: %d\n", n);

        // 만약 이벤트가 발생했다면 발생한 이벤트의 수만큼
        // 돌면서 데이터를 읽어 옵니다. 
        for (i = 0; i < n; i++)
        {
            // 만약 이벤트가 듣기 소켓에서 발생한 거라면
            // accept를 이용해서 연결 소켓을 생성한다. 
            if (events[i].data.fd == sfd)
            {
                msgst ms;
                char *msg = "Welcome LightChat World!";

                cfd = accept(sfd, (SA *)&clientaddr, &clilen);
                ev.events = EPOLLIN;
                ev.data.fd = cfd;
                epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev);

                ms.state = MSG_ALAM_STATE;
                strcpy(ms.message, msg);
                ms.message[strlen(msg)] = '\0';
                write(cfd, (char *)&ms, sizeof(msgst));
            }
            // 연결소켓에서 이벤트가 발생했다면
            // 데이터를 읽는다.
            else
            {
                msgst rcv_ms;
                ud user_data;
                int hash;
                struct user_list_node *node;

                readn = read(events[i].data.fd, (char *)&rcv_ms, 1024);
                if (readn <= 0)
                {
                    // 끊어진 소켓에 대한 아이디를 구한다.
                    for(hash = 0; hash < USER_HASH_SIZE; hash++) {
                        list_for_each_entry(node, &usr_list[hash], list) {
                            if(node->data.sock == events[i].data.fd) {
                                strcpy(rcv_ms.id, node->data.id);
                                break;
                            }
                        }
                    }

                    user_data.sock = events[i].data.fd;
                    strcpy(user_data.id, rcv_ms.id);
                    delete_usr_list(user_data);
                    epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, events);
                    printf("Log out user(%s)\n", rcv_ms.id);

                    // 끊어진 사용자 정보를 다른 사용자들에게 알린다.
                    rcv_ms.state = MSG_DELUSER_STATE;
                    for(hash = 0; hash < USER_HASH_SIZE; hash++) {
                        list_for_each_entry(node, &usr_list[hash], list) {
                            write(node->data.sock, (char *)&rcv_ms, sizeof(msgst));
                        }
                    }
                    close(events[i].data.fd);
                }
                else
                {
                    ud usr_data;
                    msgst tmp_ms;
                    char users[MESSAGE_BUFFER_SIZE];
                    int idx = 0;

                    switch(rcv_ms.state) {
                        case MSG_NEWCONNECT_STATE:
                            printf("Log in user(%s)\n", rcv_ms.id);
                            usr_data.sock = events[i].data.fd;
                            strcpy(usr_data.id, rcv_ms.id);

                            if(exist_usr_list(usr_data)) {
                                printf("exist user!\n");
                                //something to do...
                            }

                            // 사용자를 리스트에 추가
                            insert_usr_list(usr_data);

                            // 접속한 사용자에게 접속된 모든 사용자의 목록을 전송한다.(ex. usr1:usr2:usr3:...:)
                            tmp_ms.state = MSG_USERLIST_STATE;
                            for(hash = 0; hash < USER_HASH_SIZE; hash++) {
                                list_for_each_entry(node, &usr_list[hash], list) {
                                    idx += sprintf(users + idx, "%s%s", node->data.id, USER_DELIM);
                                }
                            }
                            users[idx] = '\0';
                            strcpy(tmp_ms.message, users);
                            write(events[i].data.fd, (char *)&tmp_ms, sizeof(msgst));

                            // 접속자들에게 새로운 사용자의 접속을 알린다.
                            rcv_ms.state = MSG_NEWUSER_STATE;
                            break;
                    }

                    // 모든 사용자에게 서버가 받은 메시지를 전달
                    for(hash = 0; hash < USER_HASH_SIZE; hash++) {
                        list_for_each_entry(node, &usr_list[hash], list) {
                            write(node->data.sock, (char *)&rcv_ms, sizeof(msgst));
                        }
                    }
                }
            }
        }
    }
}

unsigned int hash_func(char *s)
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31 * hashval;

    return hashval % USER_HASH_SIZE;
}

void init_usr_list()
{
    int i;

    for(i = 0; i < USER_HASH_SIZE; i++) {
        INIT_LIST_HEAD(&usr_list[i]);
    }
}

void insert_usr_list(ud data)
{
    struct user_list_node *node;
    unsigned int hash;

    node = (struct user_list_node *)malloc(sizeof(struct user_list_node));
    node->data.sock = data.sock;
    strcpy(node->data.id, data.id);
    hash = hash_func(data.id);
    list_add(&node->list, &usr_list[hash]);
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
        }
    }
}

int exist_usr_list(ud data)
{
    struct user_list_node *node;
    unsigned int hash;

    hash = hash_func(data.id);
    list_for_each_entry(node, &usr_list[hash], list) {
        if(!strcmp(data.id, node->data.id)) {
            return 1;
        }
    }

    return 0;
}
