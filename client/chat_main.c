#include <chat_main.h>
#include <motd.h>

static int term_y = 0, term_x = 0;
WINDOW *info_win, *show_win, *ulist_win, *chat_win;
struct cp_win_ui info_ui, show_ui, ulist_ui, chat_ui;
int sock;
pthread_t rcv_pthread, info_win_pthread;
int usr_state;
char time_buf[TIME_BUFFER_SIZE];
char id[ID_SIZE];
char plugin_file[FILE_NAME_MAX];

pthread_mutex_t msg_list_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t usr_list_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t info_list_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t chat_win_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t info_win_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t show_win_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ulist_win_lock = PTHREAD_MUTEX_INITIALIZER;

unsigned int msg_count;
unsigned int usr_count;
unsigned int info_count;

LIST_HEAD(msg_list);
LIST_HEAD(usr_list);
LIST_HEAD(info_list);

int main(int argc, char *argv[])
{
    int thr_id;
    char str[MESSAGE_BUFFER_SIZE];
    char srvname[SERVER_NAME_SIZE];
    char *first_scr = "Enter your id: ";
    char *srv_name_scr = "Server Name: ";
    char *time_msg_scr = "Acess Time:";
    char *current_time_scr = time_buf;
    char *argv_parse;
    msgst ms;

    // Locale 환경 셋팅
    set_env();
    // Ncurses 환경 초기화
    initscr();
    // cperl-chat init
    init_cp_chat();

    // 첫 실행 화면 출력
    mvwprintw(stdscr, term_y/2 - 8, (term_x - strlen(first_scr))/2 - 16, motd_1);
    mvwprintw(stdscr, term_y/2 - 7, (term_x - strlen(first_scr))/2 - 16, motd_2);
    mvwprintw(stdscr, term_y/2 - 6, (term_x - strlen(first_scr))/2 - 16, motd_3);
    mvwprintw(stdscr, term_y/2 - 5, (term_x - strlen(first_scr))/2 - 16, motd_4);
    mvwprintw(stdscr, term_y/2 - 4, (term_x - strlen(first_scr))/2 - 16, motd_5);
    mvwprintw(stdscr, term_y/2 - 3, (term_x - strlen(first_scr))/2 - 16, motd_6);
    mvwprintw(stdscr, term_y/2, (term_x - strlen(first_scr))/2, first_scr); 
    mvwprintw(stdscr, term_y/2 + 2, (term_x - strlen(srv_name_scr))/2 - 1, srv_name_scr);
    mvwprintw(stdscr, term_y/2 + 4, (term_x - strlen(srv_name_scr))/2 - 1, time_msg_scr);
    mvwprintw(stdscr, term_y/2 + 4, (term_x - strlen(srv_name_scr))/2 + 12, current_time_scr);

    //커서를 맨앞으로 이동
    wmove(stdscr, term_y/2, ((term_x - strlen(first_scr))/2) + strlen(first_scr) + 1);
    // 아이디 값을 받아서 저장
    getstr(id);
    memcpy(ms.id, id, strlen(id));
    ms.id[strlen(id)] = '\0';
    //커서를 맨앞으로 이동
    wmove(stdscr, term_y/2 + 2, ((term_x - strlen(srv_name_scr))/2 - 1) + strlen(srv_name_scr) +1);
    getstr(srvname);

    // 로그&Top10 출력창 생성
    info_win = create_window(info_ui);
    // 메시지 출력창 생성
    show_win = create_window(show_ui);
    // 사용자 목록창 생성 
    ulist_win = create_window(ulist_ui);
    // 사용자 입력창 생성
    chat_win = create_window(chat_ui);

    thr_id = pthread_create(&info_win_pthread, NULL, info_win_thread, NULL);
    if(thr_id < 0) {
        print_error("pthread_create error");
        return -1;
    }

    connect_server();

    while(1) {
        // 입력이 완료되면 문자열을 화면에서 지우기 위해 사용자 입력창을 재 생성 한다.
        update_chat_win();
        // 사용자의 입력을 받음.
        mvwgetstr(chat_win, 1, 1, str);

        // 아무값이 없는 입력은 무시
        if(!strlen(str)) 
            continue;

        if(str[0] == '/') {
            if(!strcmp("/connect", str)) {
                // 이미 사용자 로그인 상태이면 접속하지 않기 위한 처리를 함.
                if(usr_state == USER_LOGIN_STATE) {
                    insert_msg_list("already connected!");
                    update_show_win();
                    continue;
                }

                // 사용자 목록 창과 리스트에 남아있는 사용자 목록을 초기화 한다.
                clear_usr_list();
                update_usr_win();

                // 접속 시도
                if(connect_server() < 0) {
                    continue;
                }

            } else if(!strcmp("/disconnect", str)) {
                // 접속을 끊기 위해 메시지를 받는 쓰레드를 종료하고 읽기/쓰기 소켓을 닫는다.
                pthread_cancel(rcv_pthread);
                shutdown(sock, SHUT_RDWR);
                // 사용자 목록 초기화
                clear_usr_list();
                update_usr_win();
                insert_msg_list("disconnected!");
                update_show_win();
                usr_state = USER_LOGOUT_STATE;

            } else if(!strncmp("/script", str, 7)) {
                char tfile[FILE_NAME_MAX], tbuf[MESSAGE_BUFFER_SIZE];

                argv_parse = strtok(str, EXEC_DELIM);
                argv_parse = strtok(NULL, EXEC_DELIM);
                sprintf(tfile, "%s%s", INFO_SCRIPT_PATH, argv_parse);
                if(!access(tfile, R_OK | X_OK)) {
                    strcpy(plugin_file, tfile);
                } else {
                    sprintf(tbuf, "error: %s cannot be loaded!", tfile);
                    insert_info_list(tbuf);
                    update_info_win();
                }

            } else if(!strcmp("/clear", str)) {
                // 메시지 출력창에 있는 메시지를 모두 지운다.
                clear_msg_list();
                update_show_win();

            } else if(!strcmp("/exit", str)) {
                break;
            } 

        } else {
            // 로그인 상태일때만 서버에 메시지를 전달하게 된다.
            if(usr_state == USER_LOGIN_STATE) {
                ms.state = MSG_DATA_STATE;
                strcpy(ms.message, str);
                write(sock, (char *)&ms, sizeof(msgst));
            }
        }
    }

    pthread_cancel(info_win_pthread);
    close(sock);
    endwin();

    return 0;
}

void print_error(char* err_msg)
{
    char buf[MESSAGE_BUFFER_SIZE];

    sprintf(buf, "%s%s", "ERROR: ", err_msg);
    insert_msg_list(buf);
    update_show_win();
}

int connect_server()
{
    struct sockaddr_in srv_addr;
    int thr_id;
    msgst ms;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(!sock) {
        print_error("socket error!\n");
        return -1;
    }
    memset(&srv_addr, 0x0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    srv_addr.sin_port = htons(atoi(SERVER_PORT));
    if(connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0) {
        print_error("connect error!\n");
        close(sock);
        return -1;
    }

    // 메시지를 받는 역할을 하는 쓰레드 생성
    thr_id = pthread_create(&rcv_pthread, NULL, rcv_thread, (void *)&sock);
    if(thr_id < 0) {
        print_error("pthread_create error");
        close(sock);
        return -1;
    }

    // TCP 접속이 완료되고 서버에게 새로운 사용자라는 것을 전달한다.
    // 이때 알리는 동시에 아이디 값을 같이 전달하게 되어 서버에서 사용자 목록에 추가되게 된다(아이디는 이미 위에서 저장됨).
    ms.state = MSG_NEWCONNECT_STATE;
    strcpy(ms.id, id);
    write(sock, (char *)&ms, sizeof(msgst));

    return 0;
}

void *rcv_thread(void *data) {
    int read_len;
    msgst ms;
    char message_buf[TOTAL_MESSAGE_SIZE];
    char *usr_id;

    while(1) {
        read_len = read(sock, (char *)&ms, sizeof(msgst));
        if(read_len <= 0) {
            pthread_exit(0);
        } else {
            // 서버로 부터 온 메시지의 종류를 구별한다.
            switch(ms.state) {
                case MSG_ALAM_STATE:
                    // 서버가 클라이언트에게 알림 메시지를 전달 받을 때
                    strcpy(message_buf, ms.message);
                    break;
                case MSG_DATA_STATE:
                    // 서버로 부터 사용자들의 메시지를 전달 받을 때
                    current_time();
                    sprintf(message_buf, "%s%s%s%s", time_buf, ms.id,MESSAGE_SEPARATOR, ms.message);
                    break;
                case MSG_USERLIST_STATE:
                    // 서버로 부터 전체 사용자 목록을 받을 때
                    usr_id = strtok(ms.message, USER_DELIM);
                    insert_usr_list(usr_id);
                    while(usr_id = strtok(NULL, USER_DELIM)) {
                        insert_usr_list(usr_id);
                    }
                    update_usr_win();
                    pthread_mutex_lock(&chat_win_lock);
                    wrefresh(chat_win);
                    pthread_mutex_unlock(&chat_win_lock);
                    usr_state = USER_LOGIN_STATE;
                    continue;
                case MSG_NEWUSER_STATE:
                    // 서버로 부터 새로운 사용자에 대한 알림.
                    if(strcmp(id, ms.id)) {
                        insert_usr_list(ms.id);
                        update_usr_win();
                        current_time();
                        sprintf(message_buf, "%s%s%s", time_buf, ms.id, "님이 입장하셨습니다!");
                        break;
                    } else {
                        continue;
                    }
                case MSG_DELUSER_STATE:
                    // 서버로 부터 연결 해제된 사용자에 대한 알림.
                    delete_usr_list(ms.id);
                    update_usr_win();
                    current_time();
                    sprintf(message_buf, "%s%s%s", time_buf, ms.id, "님이 퇴장하셨습니다!");
                    break;
            }

            // 서버로 부터 받은 메시지를 가공 후 메시지 출력창에 업데이트.
            insert_msg_list(message_buf);
            update_show_win();
            pthread_mutex_lock(&chat_win_lock);
            wrefresh(chat_win);
            pthread_mutex_unlock(&chat_win_lock);
        }
    }
}

WINDOW *create_window(struct cp_win_ui ui)
{
    WINDOW *win;

    win = newwin(ui.lines, ui.cols, ui.start_y, ui.start_x);
    box(win, 0, 0);
    wborder(win, ui.left, ui.right, ui.top, ui.bottom, 
                    ui.ltop, ui.rtop, ui.lbottom, ui.rbottom);
    wrefresh(win);

    return win;
}

void set_env()
{
    char *lc;

    lc = getenv("LC_CTYPE");
    if(lc != NULL) {
        setlocale(LC_CTYPE, lc);
    } else if(lc = getenv("LC_ALL")) {
        setlocale(LC_CTYPE, lc);
    } else {
        setlocale(LC_CTYPE, "");
    }
}

void insert_info_list(char *info)
{
    struct info_list_node *node;

    node = (struct info_list_node *)malloc(sizeof(struct info_list_node));
    strcpy(node->message, info);

    pthread_mutex_lock(&info_list_lock);
    list_add(&node->list, &info_list);
    info_count++;
    pthread_mutex_unlock(&info_list_lock);
}

void clear_info_list()
{
    struct info_list_node *node, *tnode;

    pthread_mutex_lock(&info_list_lock);
    list_for_each_entry_safe(node, tnode, &info_list, list) {
        list_del(&node->list);
        free(node);
        info_count--;
    }
    pthread_mutex_unlock(&info_list_lock);
}

void update_info_win()
{
    int i = 0, cline = 0, line_max = 0;
    struct info_list_node *node, *tnode;

    pthread_mutex_lock(&info_win_lock);
    redraw_win_ui(info_win, info_ui);
    line_max = info_ui.lines - 1;

    pthread_mutex_lock(&info_list_lock);
    list_for_each_entry_safe(node, tnode, &info_list, list) {
        if(++cline >= line_max) {
            list_del(&node->list);
            free(node);
            continue;
        }
        mvwprintw(info_win, (info_ui.lines - 2) - (i++), 1, node->message);
    }
    pthread_mutex_unlock(&info_list_lock);

    wrefresh(info_win);
    pthread_mutex_unlock(&info_win_lock);
}

void insert_msg_list(char *msg)
{
    struct msg_list_node *node, *tnode;

    node = (struct msg_list_node *)malloc(sizeof(struct msg_list_node));
    strcpy(node->message, msg);

    pthread_mutex_lock(&msg_list_lock);
    list_add(&node->list, &msg_list);
    msg_count++;
    pthread_mutex_unlock(&msg_list_lock);
}

void clear_msg_list()
{
    struct msg_list_node *node, *tnode;

    pthread_mutex_lock(&msg_list_lock);
    list_for_each_entry_safe(node, tnode, &msg_list, list) {
        list_del(&node->list);
        free(node);
        msg_count--;
    pthread_mutex_unlock(&msg_list_lock);
    }
}

void update_show_win()
{
    int i = 0, cline = 0, line_max = 0;
    struct msg_list_node *node, *tnode;

    pthread_mutex_lock(&show_win_lock);
    redraw_win_ui(show_win, show_ui);
    line_max = show_ui.lines - 1;

    pthread_mutex_lock(&msg_list_lock);
    list_for_each_entry_safe(node, tnode, &msg_list, list) {
        if(++cline >= line_max) {
            list_del(&node->list);
            free(node);
            continue;
        }
        mvwprintw(show_win, (show_ui.lines - 2) - (i++), 1, node->message);
    }
    pthread_mutex_unlock(&msg_list_lock);

    wrefresh(show_win);
    pthread_mutex_unlock(&show_win_lock);
}

void insert_usr_list(char *id)
{
    struct usr_list_node *node;

    node = (struct usr_list_node *)malloc(sizeof(struct usr_list_node));
    strcpy(node->id, id);

    pthread_mutex_lock(&usr_list_lock);
    list_add(&node->list, &usr_list);
    usr_count++;
    pthread_mutex_unlock(&usr_list_lock);
}

void delete_usr_list(char* id)
{
    struct usr_list_node *pos;

    pthread_mutex_lock(&usr_list_lock);
    list_for_each_entry(pos, &usr_list, list) {
        if(!strcmp(pos->id, id)) {
            list_del(&pos->list);
            free(pos);
            pthread_mutex_unlock(&usr_list_lock);
            return;
        }
    }
    pthread_mutex_unlock(&usr_list_lock);
}

void clear_usr_list()
{
    struct usr_list_node *node, *tnode;

    pthread_mutex_lock(&usr_list_lock);
    list_for_each_entry_safe(node, tnode, &usr_list, list) {
        list_del(&node->list);
        free(node);
        usr_count--;
    }
    pthread_mutex_unlock(&usr_list_lock);
}

void update_usr_win()
{
    int i = 0, cline = 0, line_max = 0;
    struct usr_list_node *node, *tnode;

    pthread_mutex_lock(&ulist_win_lock);
    redraw_win_ui(ulist_win, ulist_ui);
    line_max = ulist_ui.lines;

    pthread_mutex_lock(&usr_list_lock);
    list_for_each_entry_safe(node, tnode, &usr_list, list) {
        if(++cline >= line_max) {
            list_del(&node->list);
            free(node);
            continue;
        }
        mvwprintw(ulist_win, 1 + (i++), 1, node->id);
    }
    pthread_mutex_unlock(&usr_list_lock);

    wrefresh(ulist_win);
    pthread_mutex_unlock(&ulist_win_lock);
}

void current_time()
{
  time_t timer;
  struct tm *t;
  int hh, mm, ss;

  timer = time(NULL);
  t = localtime(&timer);
  hh = t->tm_hour;
  mm = t->tm_min;
  ss = t->tm_sec;

  sprintf(time_buf, "[%d:%d:%d]", hh, mm, ss);
}

//info window에 정보를 출력.
void *info_win_thread(void *data) 
{
    int n, fd;
    int state;
    char buf[MESSAGE_BUFFER_SIZE];
    char *parse;
    char *file_name  = INFO_PIPE_FILE;
    struct timeval timeout = { .tv_sec = 60, .tv_usec = 0 };

    fd_set readfds, writefds;

    //fifo 파일의 존재 확인
    if(!access(file_name, F_OK)) {
        unlink(file_name);
    }

    if(mkfifo(file_name, 0644) < 0) {
        perror("mkfifo error : ");
    }

    if((fd = open(file_name, O_RDWR)) == -1 ) {
        perror("file open error : ");
    }

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    //select fifo 파일변화 추적
    while(1) {
        system(plugin_file);
        state = select(fd + 1, &readfds, NULL, NULL, &timeout);
        switch(state) {
            case -1:
                perror("select error : ");
                exit(0);
                break;

            default :
                if(FD_ISSET(fd, &readfds)) {
                    memset(buf, 0x00, MESSAGE_BUFFER_SIZE);
                    if(read(fd, buf, MESSAGE_BUFFER_SIZE) < 0) {
                        break;
                    }

                    parse = strtok(buf, DELIM);
                    insert_info_list(parse);
                    update_info_win();
                    pthread_mutex_lock(&chat_win_lock);
                    wrefresh(chat_win);
                    pthread_mutex_unlock(&chat_win_lock);
                    while(parse = strtok(NULL, DELIM)) {
                        sleep(1);
                        insert_info_list(parse);
                        update_info_win();
                        pthread_mutex_lock(&chat_win_lock);
                        wrefresh(chat_win);
                        pthread_mutex_unlock(&chat_win_lock);
                    }
                }
        }
    }
}

void update_chat_win()
{
    pthread_mutex_lock(&chat_win_lock);
    redraw_win_ui(chat_win, chat_ui);
    wmove(chat_win, 1, 1);
    wrefresh(chat_win);
    pthread_mutex_unlock(&chat_win_lock);
}

void resize_handler(int sig)
{
    struct winsize w;

    endwin();
    initscr();
    clear();
    refresh();

    ioctl(0, TIOCGWINSZ, &w);

    term_y = w.ws_row;
    term_x = w.ws_col;

    update_win_ui();

    update_show_win();
    update_usr_win();
    update_info_win();
    update_chat_win();
}

void update_win_ui()
{
    show_ui.lines = (int)((term_y * 70)/100) - 3;
    show_ui.cols = (term_x - 17);
    show_ui.start_y = (int)((term_y * 30)/100);
    show_ui.start_x = 16;
    show_ui.left = '|';
    show_ui.right= '|';
    show_ui.top = '-';
    show_ui.bottom = '-';
    show_ui.ltop = '+';
    show_ui.rtop = '+';
    show_ui.lbottom = '+';
    show_ui.rbottom = '+';

    info_ui.lines = (int)((term_y * 30)/100);
    info_ui.cols = (term_x - 17);
    info_ui.start_y = 0;
    info_ui.start_x = 16;
    info_ui.left = '|';
    info_ui.right= '|';
    info_ui.top = '-';
    info_ui.bottom = '-';
    info_ui.ltop = '+';
    info_ui.rtop = '+';
    info_ui.lbottom = '+';
    info_ui.rbottom = '+';

    ulist_ui.lines = (term_y - 4);
    ulist_ui.cols = 15;
    ulist_ui.start_y = 0;
    ulist_ui.start_x = 0;
    ulist_ui.left = '|';
    ulist_ui.right= '|';
    ulist_ui.top = '-';
    ulist_ui.bottom = '-';
    ulist_ui.ltop = '+';
    ulist_ui.rtop = '+';
    ulist_ui.lbottom = '+';
    ulist_ui.rbottom = '+';

    chat_ui.lines = 3;
    chat_ui.cols = (term_x - 1);
    chat_ui.start_y = (term_y - 3);
    chat_ui.start_x = 0;
    chat_ui.left = '|';
    chat_ui.right= '|';
    chat_ui.top = '-';
    chat_ui.bottom = '-';
    chat_ui.ltop = '+';
    chat_ui.rtop = '+';
    chat_ui.lbottom = '+';
    chat_ui.rbottom = '+';
}

void init_cp_chat()
{
    current_time();

    signal(SIGWINCH, resize_handler);

    // 처음 사용자의 상태를 로그아웃 상태로 셋팅
    usr_state = USER_LOGOUT_STATE;

    // 초기 플러그인 스크립트 이름 정의
    sprintf(plugin_file, "%s%s", INFO_SCRIPT_PATH, "naver_rank");

    term_y = LINES;
    term_x = COLS;

    update_win_ui();
}

void redraw_win_ui(WINDOW *win, struct cp_win_ui ui)
{
    werase(win);
    wresize(win, ui.lines, ui.cols);
    mvwin(win, ui.start_y, ui.start_x);
    wborder(win, ui.right, ui.left, ui.top, ui.bottom, 
                    ui.ltop, ui.rtop, ui.lbottom, ui.rbottom);
}
