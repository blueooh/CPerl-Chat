#define TEST
#define INFO_PIPE_FILE "/tmp/info_pipe"
#define MESSAGE_SEPARATOR ": "
#define DELIM "/"
#define USER_DELIM ":"
#define EXEC_DELIM " "
#define SERVER_NAME_SIZE 30
#define TIME_BUFFER_SIZE 10
#define ID_SIZE 50
#define MESSAGE_BUFFER_SIZE 512
#define TOTAL_MESSAGE_SIZE \
    (TIME_BUFFER_SIZE + ID_SIZE + MESSAGE_BUFFER_SIZE)

enum {
    MSG_ALAM_STATE = 0,
    MSG_DATA_STATE,
    MSG_NEWCONNECT_STATE,
    MSG_NEWUSER_STATE,
    MSG_USERLIST_STATE,
    MSG_DELUSER_STATE,
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

