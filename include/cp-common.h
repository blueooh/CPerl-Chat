#ifndef __CP_COMMON_H__
#define __CP_COMMON_H__

//#define TEST
#define MESSAGE_SEPARATOR ": "
#define DELIM "/"
#define USER_DELIM ":"
#define EXEC_DELIM " "
#define SERVER_NAME_SIZE 30
#define TIME_BUFFER_SIZE 11
#define ID_SIZE 50
#define VERSION_SIZE 10
#define MESSAGE_BUFFER_SIZE 512
#define FILE_NAME_MAX 255
#define FILE_PATH_MAX 4096
#define TOTAL_MESSAGE_SIZE \
    (TIME_BUFFER_SIZE + ID_SIZE + MESSAGE_BUFFER_SIZE)
#define USER_HASH_SIZE 20

enum {
    MSG_ALAM_STATE = 0,
    MSG_DATA_STATE,
    MSG_NEWCONNECT_STATE,
    MSG_RECONNECT_STATE,
    MSG_NEWUSER_STATE,
    MSG_USERLIST_STATE,
    MSG_DELUSER_STATE,
    MSG_ERROR_STATE,
    MSG_AVAILTEST_STATE,
};

enum {
    USER_LOGOUT_STATE = 0,
    USER_LOGIN_STATE
};

typedef struct cp_packet_header {
    char version[VERSION_SIZE];
    unsigned int state;
    char id[ID_SIZE];
}CP_PACKET_HEADER;

typedef struct cp_packet {
    CP_PACKET_HEADER cp_h;
    char message[MESSAGE_BUFFER_SIZE];
}CP_PACKET;

inline unsigned int hash_func(char *s)
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31 * hashval;

    return hashval % USER_HASH_SIZE;
}
#endif
