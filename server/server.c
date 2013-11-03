#include "server.h"

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

    // 이벤트 풀의 크기만큼 events구조체를 생성한다.
    events = (struct epoll_event *)malloc(sizeof(*events) * EPOLL_SIZE);

    ulist *user_list = (ulist *)malloc(sizeof(ulist));
    init_ulist(user_list);

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
    addr.sin_port = htons(atoi(argv[1]));
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
                char *msg = "Connect Success!";

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
                msgst ms;
                readn = read(events[i].data.fd, (char *)&ms, 1024);
                if (readn <= 0)
                {
                    p_unode cnode = user_list->head;
                    ms.state = MSG_DELUSER_STATE;
                    while(cnode) {
                        if(cnode->usr_data.sock == events[i].data.fd) {
                            strcpy(ms.id, cnode->usr_data.id);
                            break;
                        }
                        cnode = cnode->next;
                    }
                    delete_ulist(user_list, events[i].data.fd);
                    epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, events);
                    printf("Log out user(%s)\n", ms.id);

                    cnode = user_list->head;
                    while(cnode) {
                        write(cnode->usr_data.sock, (char *)&ms, sizeof(msgst));
                        cnode = cnode->next;
                    }
                    close(events[i].data.fd);
                }
                else
                {
                    ud usr_data;
                    p_unode cnode;
                    msgst tmp_ms;

                    switch(ms.state) {
                        case MSG_NEWUSER_STATE:
                            printf("Log in user(%s)\n", ms.id);
                            usr_data.sock = events[i].data.fd;
                            strcpy(usr_data.id, ms.id);
                            insert_ulist(user_list, usr_data);

                            cnode = user_list->head;
                            tmp_ms.state = MSG_NEWUSER_STATE;

                            while(cnode) {
                                strcpy(tmp_ms.id, cnode->usr_data.id);
                                write(usr_data.sock, (char *)&tmp_ms, sizeof(msgst));
                                cnode = cnode->next;
                            }
                            tmp_ms.state = MSG_ENDUSER_STATE;
                            write(usr_data.sock, (char *)&tmp_ms, sizeof(msgst));

                            cnode = user_list->head;

                            while(cnode) {
                                if(cnode->usr_data.sock != events[i].data.fd) {
                                    write(cnode->usr_data.sock, (char *)&ms, sizeof(msgst));
                                }
                                cnode = cnode->next;
                            }
                            continue;
                    }

                    cnode = user_list->head;

                    while(cnode) {
                        write(cnode->usr_data.sock, (char *)&ms, sizeof(msgst));
                        cnode = cnode->next;
                    }
                }
            }
        }
    }
}

void init_ulist(ulist *lptr)
{
    lptr->count = 0;
    lptr->head = NULL;
    lptr->tail = NULL;
}

void insert_ulist(ulist *lptr, ud data)
{
    p_unode tmp_nptr;
    p_unode new_nptr = (p_unode)malloc(sizeof(unode));

    memcpy(&new_nptr->usr_data, &data, sizeof(ud));

    if(!lptr->head) {
        new_nptr->next = NULL;
        new_nptr->prev = NULL;
        lptr->head = lptr->tail = new_nptr;
    } else {
        new_nptr->prev = lptr->tail;
        new_nptr->next = NULL;
        lptr->tail->next = new_nptr;
        lptr->tail = new_nptr;
    }

    lptr->count++;
}

void delete_ulist(ulist *lptr, int key)
{
    p_unode cnode = lptr->head, tnode;

    while(cnode) {
        if(cnode->usr_data.sock == key) {
            if(!cnode->prev) {
                if(!cnode->next) {
                    lptr->head = NULL;
                    lptr->tail = NULL;
                } else {
                    lptr->head = lptr->tail = cnode->next;
                    cnode->next->prev = NULL;
                }
            } else {
                if(!cnode->next) {
                    cnode->prev->next = NULL;
                    lptr->tail = cnode->prev;
                } else {
                    cnode->next->prev = cnode->prev;
                    cnode->prev->next = cnode->next;
                }
            }
            lptr->count--;
            free(cnode);
            return;
        }
        cnode = cnode->next;
    }
}
