//#define TEST
#define MESSAGE_SEPARATOR ": "
#define DELIM "/"
#define USER_DELIM ":"
#define EXEC_DELIM " "
#define SERVER_NAME_SIZE 30
#define TIME_BUFFER_SIZE 11
#define ID_SIZE 50
#define MESSAGE_BUFFER_SIZE 512
#define FILE_NAME_MAX 255
#define FILE_PATH_MAX 4096
#define TOTAL_MESSAGE_SIZE \
    (TIME_BUFFER_SIZE + ID_SIZE + MESSAGE_BUFFER_SIZE)

/* macro to format string with variable args, using dynamic buffer size */
#define cpchat_va_format(__format)                                     \
    va_list argptr;                                                     \
    int vaa_size, vaa_num;                                              \
    char *vbuffer, *vaa_buffer2;                                        \
    vaa_size = 1024;                                                    \
    vbuffer = malloc (vaa_size);                                        \
    if (vbuffer)                                                        \
    {                                                                   \
        while (1)                                                       \
        {                                                               \
            va_start (argptr, __format);                                \
            vaa_num = vsnprintf (vbuffer, vaa_size, __format, argptr);  \
            va_end (argptr);                                            \
            if ((vaa_num >= 0) && (vaa_num < vaa_size))                 \
                break;                                                  \
            vaa_size = (vaa_num >= 0) ? vaa_num + 1 : vaa_size * 2;     \
            vaa_buffer2 = realloc (vbuffer, vaa_size);                  \
            if (!vaa_buffer2)                                           \
            {                                                           \
                free (vbuffer);                                         \
                vbuffer = NULL;                                         \
                break;                                                  \
            }                                                           \
            vbuffer = vaa_buffer2;                                      \
        }                                                               \
    }

enum {
    MSG_ALAM_STATE = 0,
    MSG_DATA_STATE,
    MSG_NEWCONNECT_STATE,
    MSG_NEWUSER_STATE,
    MSG_USERLIST_STATE,
    MSG_DELUSER_STATE,
    MSG_ERROR_STATE,
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
