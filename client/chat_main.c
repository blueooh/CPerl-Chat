#include <chat_main.h>
#include <motd.h>

struct cp_win_manage cw_manage[CP_MAX_WIN];
static int term_y = 0, term_x = 0;
int sock;
struct linger ling;
pthread_t rcv_pthread, info_win_pthread, local_info_win_pthread;
int usr_state;
char time_buf[TIME_BUFFER_SIZE];
char id[ID_SIZE];
char srvname[SERVER_NAME_SIZE];
char plugin_cmd[MESSAGE_BUFFER_SIZE];

struct cp_chat_options options[] = {
    {CP_OPT_HELP, "help", 4, "/help [no argument]: Show CPerl-Chat help messages"},
    {CP_OPT_CONNECT, "connect", 7, "/connect [no argument]: Try connect to server"},
    {CP_OPT_DISCONNECT, "disconnect", 10, "/disconnect [no argument]: Try disconnect from server"},
    {CP_OPT_SCRIPT, "script", 6, "/script [script name]: Excute script you made to plugin"},
    {CP_OPT_CLEAR, "clear", 5, "/clear [no argument]: Clear messages in show window"},
    {CP_OPT_EXIT, "exit", 4, "/exit [no argument]: Exit program"},
};

pthread_mutex_t msg_list_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t usr_list_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t info_list_lock = PTHREAD_MUTEX_INITIALIZER;

unsigned int msg_count;
unsigned int usr_count;
unsigned int info_count;

LIST_HEAD(msg_list);
LIST_HEAD(usr_list);
LIST_HEAD(info_list);

int main(int argc, char *argv[])
{
    int thr_id;

    // Locale 환경 셋팅
    set_env();
    // Ncurses 환경 초기화
    initscr();
    start_color();
    // cperl-chat init
    cp_init_chat();
    // 첫 실행 화면
    first_scr_ui();
    // CPerl-Chat 윈도우 생성
    clear();
    refresh();
    cp_create_win();

    thr_id = pthread_create(&info_win_pthread, NULL, info_win_thread, NULL);
    if(thr_id < 0) {
        print_error("pthread_create error");
        return -1;
    }
    thr_id = pthread_create(&local_info_win_pthread, NULL, local_info_win_thread, NULL);
    if(thr_id < 0) {
        print_error("pthread_create error");
        return -1;
    }

    connect_server();

    while(1) {
        msgst ms;
        char *argv_parse;
        char str[MESSAGE_BUFFER_SIZE];
        char *cur_opt;

        // 입력이 완료되면 문자열을 화면에서 지우기 위해 사용자 입력창을 재 생성 한다.
        cw_manage[CP_CHAT_WIN].update_handler();
        // 사용자의 입력을 받음.
        mvwgetstr(cw_manage[CP_CHAT_WIN].win, 1, 1, str);

        // 아무값이 없는 입력은 무시
        if(!strlen(str)) 
            continue;

        if(str[0] == '/') {
            cur_opt = str + 1;
            if(cp_option_check(cur_opt, CP_OPT_HELP, false)) {
                int i;
                for(i = 0; i < CP_OPT_MAX; i++) {
                    insert_msg_list(MSG_ALAM_STATE, "", options[i].op_desc);
                }
                cw_manage[CP_SHOW_WIN].update_handler();
                continue;
            } else if(cp_option_check(cur_opt, CP_OPT_CONNECT, false)) {
                // 이미 사용자 로그인 상태이면 접속하지 않기 위한 처리를 함.
                if(usr_state == USER_LOGIN_STATE) {
                    insert_msg_list(MSG_ALAM_STATE, "", "already connected!");
                    cw_manage[CP_SHOW_WIN].update_handler();
                    continue;
                }

                // 사용자 목록 창과 리스트에 남아있는 사용자 목록을 초기화 한다.
                clear_usr_list();
                cw_manage[CP_ULIST_WIN].update_handler();

                // 접속 시도
                if(connect_server() < 0) {
                    continue;
                }

            } else if(cp_option_check(cur_opt, CP_OPT_DISCONNECT, false)) {
                // 접속을 끊기 위해 메시지를 받는 쓰레드를 종료하고 읽기/쓰기 소켓을 닫는다.
                pthread_cancel(rcv_pthread);
                close(sock);
                // 사용자 목록 초기화
                clear_usr_list();
                cw_manage[CP_ULIST_WIN].update_handler();
                insert_msg_list(MSG_ERROR_STATE, "", "disconnected!");
                cw_manage[CP_SHOW_WIN].update_handler();
                usr_state = USER_LOGOUT_STATE;

            } else if(cp_option_check(cur_opt, CP_OPT_SCRIPT, true)) {
                char tfile[FILE_NAME_MAX], tbuf[MESSAGE_BUFFER_SIZE];

                argv_parse = strtok(str, EXEC_DELIM);
                argv_parse = strtok(NULL, EXEC_DELIM);
                sprintf(tfile, "%s%s", INFO_SCRIPT_PATH, argv_parse);
                if(!access(tfile, R_OK | X_OK)) {
                    sprintf(plugin_cmd, "%s %s", tfile, INFO_PIPE_FILE);
                } else {
                    sprintf(tbuf, "error: %s cannot be loaded!", tfile);
                    insert_info_list(tbuf, COLOR_PAIR(3));
                    cw_manage[CP_INFO_WIN].update_handler();
                }

            } else if(cp_option_check(cur_opt, CP_OPT_CLEAR, false)) {
                // 메시지 출력창에 있는 메시지를 모두 지운다.
                clear_msg_list();
                cw_manage[CP_SHOW_WIN].update_handler();

            } else if(cp_option_check(cur_opt, CP_OPT_EXIT, false)) {
                break;
            } 

        } else {
            // 로그인 상태일때만 서버에 메시지를 전달하게 된다.
            if(usr_state == USER_LOGIN_STATE) {
                ms.state = MSG_DATA_STATE;
                memcpy(ms.id, id, strlen(id));
                ms.id[strlen(id)] = '\0';
                strcpy(ms.message, str);
                write(sock, (char *)&ms, sizeof(msgst));
            }
        }
    }

    pthread_cancel(rcv_pthread);
    pthread_cancel(info_win_pthread);
    pthread_cancel(local_info_win_pthread);
    close(sock);
    unlink(INFO_PIPE_FILE);
    endwin();

    return 0;
}

void print_error(char* err_msg)
{
    char buf[MESSAGE_BUFFER_SIZE];

    sprintf(buf, "%s%s", "ERROR: ", err_msg);
    insert_msg_list(MSG_ERROR_STATE, "", buf);
    cw_manage[CP_SHOW_WIN].update_handler();
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

    ling.l_onoff = 1;
    ling.l_linger = 0;

    setsockopt(sock, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

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
    char message_buf[MESSAGE_BUFFER_SIZE];
    char *usr_id, *pbuf;

    while(1) {
        read_len = read(sock, (char *)&ms, sizeof(msgst));
        if(read_len <= 0) {
            close(sock);
            usr_state = USER_LOGOUT_STATE; 
            clear_usr_list();
            cw_manage[CP_ULIST_WIN].update_handler();
            insert_msg_list(MSG_ERROR_STATE, "", "connection closed with server!");
            cw_manage[CP_SHOW_WIN].update_handler();
            pthread_cancel(rcv_pthread);
        } else {
            // 서버로 부터 온 메시지의 종류를 구별한다.
            switch(ms.state) {
                    // 서버가 클라이언트에게 알림 메시지를 전달 받을 때
                case MSG_ALAM_STATE:
                    // 서버로 부터 사용자들의 메시지를 전달 받을 때
                case MSG_DATA_STATE:
                    strcpy(message_buf, ms.message);
                    break;
                    // 서버로 부터 전체 사용자 목록을 받을 때
                case MSG_USERLIST_STATE:
                    pbuf = ms.message;
                    while(usr_id = strtok(pbuf, USER_DELIM)) {
                        insert_usr_list(usr_id);
                        pbuf = NULL;
                    }
                    cw_manage[CP_ULIST_WIN].update_handler();
                    wrefresh(cw_manage[CP_CHAT_WIN].win);
                    usr_state = USER_LOGIN_STATE;
                    continue;
                    // 서버로 부터 새로운 사용자에 대한 알림.
                case MSG_NEWUSER_STATE:
                    if(strcmp(id, ms.id)) {
                        insert_usr_list(ms.id);
                        cw_manage[CP_ULIST_WIN].update_handler();
                        break;
                    } else {
                        continue;
                    }
                    // 서버로 부터 연결 해제된 사용자에 대한 알림.
                case MSG_DELUSER_STATE:
                    delete_usr_list(ms.id);
                    cw_manage[CP_ULIST_WIN].update_handler();
                    break;
            }

            // 서버로 부터 받은 메시지를 가공 후 메시지 출력창에 업데이트.
            insert_msg_list(ms.state, ms.id, message_buf);
            cw_manage[CP_SHOW_WIN].update_handler();
            wrefresh(cw_manage[CP_CHAT_WIN].win);
        }
    }
}

WINDOW *create_window(struct win_ui ui)
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

void insert_info_list(char *info, int attrs)
{
    struct info_list_node *node;

    node = (struct info_list_node *)malloc(sizeof(struct info_list_node));
    strcpy(node->message, info);
    node->attrs = attrs;

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
    WINDOW *win = cw_manage[CP_INFO_WIN].win;
    struct win_ui *ui = &cw_manage[CP_INFO_WIN].ui;
    int i = 0, cline = 0, line_max = 0;
    int print_y, print_x;
    struct info_list_node *node, *tnode;

    line_max = ui->lines - 1;

    werase(win);

    pthread_mutex_lock(&info_list_lock);
    list_for_each_entry_safe(node, tnode, &info_list, list) {
        print_y = (ui->lines - 2) - i;
        print_x = 1;

        if(++cline >= line_max) {
            list_del(&node->list);
            free(node);
            continue;
        }

        wattron(win, node->attrs);
        mvwprintw(win, print_y, print_x, node->message);
        wattroff(win, node->attrs);
        i++;
    }
    pthread_mutex_unlock(&info_list_lock);

    draw_win_ui(win, *ui);
}

void update_local_info_win()
{
    WINDOW *win = cw_manage[CP_LO_INFO_WIN].win;
    int print_y, print_x;
    glibtop_mem memory;

    print_y = 1;
    print_x = 1;

    werase(win);

    glibtop_get_mem(&memory);

    mvwprintw(win, print_y++, print_x, "Memory      : %ld MB / %ld MB", 
            (unsigned long)memory.total/(1024*1024), (unsigned long)memory.used/(1024*1024));

    draw_win_ui(win, cw_manage[CP_LO_INFO_WIN].ui);
}

void insert_msg_list(int msg_type, char *usr_id, char *msg)
{
    struct msg_list_node *node, *tnode;

    node = (struct msg_list_node *)malloc(sizeof(struct msg_list_node));
    node->type = msg_type;
    current_time();
    strcpy(node->time, time_buf);
    strcpy(node->id, usr_id);
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
    WINDOW *win = cw_manage[CP_SHOW_WIN].win;
    struct win_ui *ui = &cw_manage[CP_SHOW_WIN].ui;
    int i = 0, cline = 0, line_max = 0;
    int print_y, print_x;
    struct msg_list_node *node, *tnode;

    line_max = ui->lines - 1;

    werase(win);

    pthread_mutex_lock(&msg_list_lock);
    list_for_each_entry_safe(node, tnode, &msg_list, list) {
        print_x = 1;
        print_y = (ui->lines - 2) - i;

        if(++cline >= line_max) {
            list_del(&node->list);
            free(node);
            continue;
        }

        switch(node->type) {
            case MSG_DATA_STATE:
                mvwprintw(win, print_y, print_x, node->time);
                wattron(win, COLOR_PAIR(1) | A_BOLD);
                mvwprintw(win, print_y, print_x += strlen(node->time), node->id);
                wattroff(win, COLOR_PAIR(1) | A_BOLD);
                wattron(win, COLOR_PAIR(2));
                mvwprintw(win, print_y, print_x += strlen(node->id), MESSAGE_SEPARATOR);
                wattroff(win, COLOR_PAIR(2));
                wattron(win, COLOR_PAIR(1));
                mvwprintw(win, print_y, print_x += strlen(MESSAGE_SEPARATOR), node->message);
                wattroff(win, COLOR_PAIR(1));
                break;

            case MSG_DELUSER_STATE:
                mvwprintw(win, print_y, print_x, node->time);
                wattron(win, COLOR_PAIR(3) | A_BOLD);
                mvwprintw(win, print_y, print_x += strlen(node->time), node->id);
                wattroff(win, COLOR_PAIR(3) | A_BOLD);
                wattron(win, COLOR_PAIR(3));
                mvwprintw(win, print_y, print_x += strlen(node->id), "님이 퇴장 하셨습니다!");
                wattroff(win, COLOR_PAIR(3));
                break;

            case MSG_NEWUSER_STATE:
                mvwprintw(win, print_y, print_x, node->time);
                wattron(win, COLOR_PAIR(2) | A_BOLD);
                mvwprintw(win, print_y, print_x += strlen(node->time), node->id);
                wattroff(win, COLOR_PAIR(2) | A_BOLD);
                wattron(win, COLOR_PAIR(2));
                mvwprintw(win, print_y, print_x += strlen(node->id), "님이 입장 하셨습니다!");
                wattroff(win, COLOR_PAIR(2));
                break;

            case MSG_ERROR_STATE:
                mvwprintw(win, print_y, print_x, node->time);
                wattron(win, COLOR_PAIR(3));
                mvwprintw(win, print_y, print_x += strlen(node->time), node->message);
                wattroff(win, COLOR_PAIR(3));
                break;

            case MSG_ALAM_STATE:
            default:
                mvwprintw(win, print_y, print_x, node->time);
                wattron(win, COLOR_PAIR(2));
                mvwprintw(win, print_y, print_x += strlen(node->time), node->message);
                wattroff(win, COLOR_PAIR(2));
                break;
        }
        i++;
    }
    pthread_mutex_unlock(&msg_list_lock);

    draw_win_ui(win, *ui);
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
    WINDOW *win = cw_manage[CP_ULIST_WIN].win;
    struct win_ui *ui = &cw_manage[CP_ULIST_WIN].ui;
    int i = 0, cline = 0, line_max = 0;
    int print_y, print_x;
    struct usr_list_node *node, *tnode;

    line_max = ui->lines;

    werase(win);

    pthread_mutex_lock(&usr_list_lock);
    list_for_each_entry_safe(node, tnode, &usr_list, list) {
        print_y = i + 1;
        print_x = 1;

        if(++cline >= line_max) {
            list_del(&node->list);
            free(node);
            continue;
        }
        wattron(win, COLOR_PAIR(4));
        mvwprintw(win, print_y, print_x, node->id);
        wattroff(win, COLOR_PAIR(4));
        i++;
    }
    pthread_mutex_unlock(&usr_list_lock);

    draw_win_ui(win, *ui);
}

void current_time()
{
  time_t timer;
  struct tm *t;
  int hh, mm, ss;
  int len;

  timer = time(NULL);
  t = localtime(&timer);
  hh = t->tm_hour;
  mm = t->tm_min;
  ss = t->tm_sec;

  len = sprintf(time_buf, "[%02d:%02d:%02d]", hh, mm, ss);
  time_buf[len] = '\0';
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
        char *pbuf;
        system(plugin_cmd);
        state = select(fd + 1, &readfds, NULL, NULL, &timeout);
        switch(state) {
            case -1:
                perror("select error : ");
                exit(0);
                break;

            default :
                if(FD_ISSET(fd, &readfds)) {
                    if(read(fd, buf, MESSAGE_BUFFER_SIZE) < 0) {
                        break;
                    }

                    pbuf = buf;
                    while(parse = strtok(pbuf, DELIM)) {
                        insert_info_list(parse, COLOR_PAIR(5));
                        cw_manage[CP_INFO_WIN].update_handler();
                        wrefresh(cw_manage[CP_CHAT_WIN].win);
                        pbuf = NULL;
                        sleep(1);
                    }
                }
        }
    }
}

void *local_info_win_thread(void *data)
{
    glibtop_init();

    while(1) {
        cw_manage[CP_LO_INFO_WIN].update_handler();
        wrefresh(cw_manage[CP_CHAT_WIN].win);
        sleep(1);
    }
}

void update_chat_win()
{
    WINDOW *win = cw_manage[CP_CHAT_WIN].win;

    werase(win);
    draw_win_ui(win, cw_manage[CP_CHAT_WIN].ui);
}

void resize_handler(int sig)
{
    int win_idx;
    struct winsize w;

    endwin();
    initscr();
    clear();
    refresh();

    ioctl(0, TIOCGWINSZ, &w);

    term_y = w.ws_row;
    term_x = w.ws_col;

    update_win_ui();

    for(win_idx= 0; win_idx < CP_MAX_WIN; win_idx++) {
        resize_win_ui(
                cw_manage[win_idx].win, 
                cw_manage[win_idx].ui, 
                cw_manage[win_idx].update_handler
                );
    }
}

void update_win_ui()
{
    cw_manage[CP_SHOW_WIN].ui.lines = (int)((term_y * 70)/100) - 3;
    cw_manage[CP_SHOW_WIN].ui.cols = (term_x - 17);
    cw_manage[CP_SHOW_WIN].ui.start_y = (int)((term_y * 30)/100);
    cw_manage[CP_SHOW_WIN].ui.start_x = 16;
    cw_manage[CP_SHOW_WIN].ui.left = '|';
    cw_manage[CP_SHOW_WIN].ui.right= '|';
    cw_manage[CP_SHOW_WIN].ui.top = '-';
    cw_manage[CP_SHOW_WIN].ui.bottom = '-';
    cw_manage[CP_SHOW_WIN].ui.ltop = '+';
    cw_manage[CP_SHOW_WIN].ui.rtop = '+';
    cw_manage[CP_SHOW_WIN].ui.lbottom = '+';
    cw_manage[CP_SHOW_WIN].ui.rbottom = '+';

    cw_manage[CP_INFO_WIN].ui.lines = (int)((term_y * 30)/100);
    cw_manage[CP_INFO_WIN].ui.cols = (term_x - 17)/2;
    cw_manage[CP_INFO_WIN].ui.start_y = 0;
    cw_manage[CP_INFO_WIN].ui.start_x = 16;
    cw_manage[CP_INFO_WIN].ui.left = '|';
    cw_manage[CP_INFO_WIN].ui.right= '|';
    cw_manage[CP_INFO_WIN].ui.top = '-';
    cw_manage[CP_INFO_WIN].ui.bottom = '-';
    cw_manage[CP_INFO_WIN].ui.ltop = '+';
    cw_manage[CP_INFO_WIN].ui.rtop = '+';
    cw_manage[CP_INFO_WIN].ui.lbottom = '+';
    cw_manage[CP_INFO_WIN].ui.rbottom = '+';

    cw_manage[CP_LO_INFO_WIN].ui.lines = (int)((term_y * 30)/100);
    cw_manage[CP_LO_INFO_WIN].ui.cols = (term_x - 17)/2;
    cw_manage[CP_LO_INFO_WIN].ui.start_y = 0;
    cw_manage[CP_LO_INFO_WIN].ui.start_x = 16 + ((term_x - 17)/2);
    cw_manage[CP_LO_INFO_WIN].ui.left = '|';
    cw_manage[CP_LO_INFO_WIN].ui.right= '|';
    cw_manage[CP_LO_INFO_WIN].ui.top = '-';
    cw_manage[CP_LO_INFO_WIN].ui.bottom = '-';
    cw_manage[CP_LO_INFO_WIN].ui.ltop = '+';
    cw_manage[CP_LO_INFO_WIN].ui.rtop = '+';
    cw_manage[CP_LO_INFO_WIN].ui.lbottom = '+';
    cw_manage[CP_LO_INFO_WIN].ui.rbottom = '+';

    cw_manage[CP_ULIST_WIN].ui.lines = (term_y - 4);
    cw_manage[CP_ULIST_WIN].ui.cols = 15;
    cw_manage[CP_ULIST_WIN].ui.start_y = 0;
    cw_manage[CP_ULIST_WIN].ui.start_x = 0;
    cw_manage[CP_ULIST_WIN].ui.left = '|';
    cw_manage[CP_ULIST_WIN].ui.right= '|';
    cw_manage[CP_ULIST_WIN].ui.top = '-';
    cw_manage[CP_ULIST_WIN].ui.bottom = '-';
    cw_manage[CP_ULIST_WIN].ui.ltop = '+';
    cw_manage[CP_ULIST_WIN].ui.rtop = '+';
    cw_manage[CP_ULIST_WIN].ui.lbottom = '+';
    cw_manage[CP_ULIST_WIN].ui.rbottom = '+';

    cw_manage[CP_CHAT_WIN].ui.lines = 3;
    cw_manage[CP_CHAT_WIN].ui.cols = (term_x - 1);
    cw_manage[CP_CHAT_WIN].ui.start_y = (term_y - 3);
    cw_manage[CP_CHAT_WIN].ui.start_x = 0;
    cw_manage[CP_CHAT_WIN].ui.left = '|';
    cw_manage[CP_CHAT_WIN].ui.right= '|';
    cw_manage[CP_CHAT_WIN].ui.top = '-';
    cw_manage[CP_CHAT_WIN].ui.bottom = '-';
    cw_manage[CP_CHAT_WIN].ui.ltop = '+';
    cw_manage[CP_CHAT_WIN].ui.rtop = '+';
    cw_manage[CP_CHAT_WIN].ui.lbottom = '+';
    cw_manage[CP_CHAT_WIN].ui.rbottom = '+';
}

void cp_init_chat()
{
    current_time();

    signal(SIGWINCH, resize_handler);

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);

    // 처음 사용자의 상태를 로그아웃 상태로 셋팅
    usr_state = USER_LOGOUT_STATE;

    // 초기 플러그인 스크립트 명령 라인 생성
    sprintf(plugin_cmd, "%s%s %s", INFO_SCRIPT_PATH, "naver_rank", INFO_PIPE_FILE);

    term_y = LINES;
    term_x = COLS;

    update_win_ui();
    reg_update_win_func();
}

void resize_win_ui(WINDOW *win, struct win_ui ui, cb_update update)
{
    werase(win);
    wresize(win, ui.lines, ui.cols);
    mvwin(win, ui.start_y, ui.start_x);
    update();
}

void draw_win_ui(WINDOW *win, struct win_ui ui)
{
    wborder(win, ui.right, ui.left, ui.top, ui.bottom, 
            ui.ltop, ui.rtop, ui.lbottom, ui.rbottom);
    wrefresh(win);
}

void reg_update_win_func()
{
    cw_manage[CP_CHAT_WIN].update_handler = update_chat_win;
    cw_manage[CP_SHOW_WIN].update_handler = update_show_win;
    cw_manage[CP_INFO_WIN].update_handler = update_info_win;
    cw_manage[CP_LO_INFO_WIN].update_handler = update_local_info_win;
    cw_manage[CP_ULIST_WIN].update_handler = update_usr_win;
}

void first_scr_ui()
{
    char *first_scr = "Enter your id: ";
    char *srv_name_scr = "Server Name: ";
    char *time_msg_scr = "Acess Time:";
    char *current_time_scr = time_buf;

    wattron(stdscr, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(stdscr, term_y/2 - 8, (term_x - strlen(motd_1))/2, motd_1);
    mvwprintw(stdscr, term_y/2 - 7, (term_x - strlen(motd_1))/2, motd_2);
    mvwprintw(stdscr, term_y/2 - 6, (term_x - strlen(motd_1))/2, motd_3);
    mvwprintw(stdscr, term_y/2 - 5, (term_x - strlen(motd_1))/2, motd_4);
    wattroff(stdscr, COLOR_PAIR(1) | A_BOLD);
    wattron(stdscr, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(stdscr, term_y/2 - 4, (term_x - strlen(motd_1))/2, motd_5);
    mvwprintw(stdscr, term_y/2 - 3, (term_x - strlen(motd_1))/2, motd_6);
    wattroff(stdscr, COLOR_PAIR(3)| A_BOLD);
    mvwprintw(stdscr, term_y/2, (term_x - strlen(first_scr))/2, first_scr); 
    mvwprintw(stdscr, term_y/2 + 2, (term_x - strlen(srv_name_scr))/2 - 1, srv_name_scr);
    mvwprintw(stdscr, term_y/2 + 4, (term_x - strlen(srv_name_scr))/2 - 1, time_msg_scr);
    mvwprintw(stdscr, term_y/2 + 4, (term_x - strlen(srv_name_scr))/2 + 12, current_time_scr);

    //커서를 맨앞으로 이동
    wmove(stdscr, term_y/2, ((term_x - strlen(first_scr))/2) + strlen(first_scr) + 1);
    // 아이디 값을 받아서 저장
    getstr(id);
    //커서를 맨앞으로 이동
    wmove(stdscr, term_y/2 + 2, ((term_x - strlen(srv_name_scr))/2 - 1) + strlen(srv_name_scr) +1);
    getstr(srvname);
}

void cp_create_win()
{
    int win_idx;
    for(win_idx = 0; win_idx < CP_MAX_WIN; win_idx++) {
        cw_manage[win_idx].win = create_window(cw_manage[win_idx].ui);
    }
}

int cp_option_check(char *option, option_type type, bool arg)
{
    if(!arg) {
        if((options[type].op_len == strlen(option)) && 
                !strncmp(options[type].op_name, option, options[type].op_len)) {
            return 1;
        }
    } else {
        if(!strncmp(options[type].op_name, option, options[type].op_len)) {
            return 1;
        }
    }

    return 0;
}
