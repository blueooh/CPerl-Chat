#include <cp-log.h>

int log_fd;

int cp_init_log(const char * file)
{
    log_fd = open(file, O_RDWR| O_APPEND | O_CREAT, 0644);
    if(!log_fd) {
        return -1;
    }

    return 0;
}

int cp_log(char *buf, ...)
{
    char log_buf[CP_LOG_BUFFER];
    int log_size;

    cp_va_format(buf);
    log_size = cp_get_log_time(log_buf, sizeof(log_buf));

    int vb_len = sizeof(vbuffer) - 1;
    if(vbuffer[vb_len] == '\n') {
        log_size += snprintf(log_buf + log_size, vaa_size, "%s", vbuffer);
    } else {
        log_size += snprintf(log_buf + log_size, vaa_size + 1, "%s\n", vbuffer);
    }

    write(log_fd, log_buf, log_size);

    free(vbuffer);

    return log_size;
}

int cp_get_log_time(char *buf, int size)
{
    int len;
    time_t now;
    struct tm cur_time;

    now = time(NULL);
    localtime_r(&now, &cur_time);

    len = snprintf(buf, size, "[%04d.%02d.%02d %02d:%02d:%02d] ",
            cur_time.tm_year+1900, cur_time.tm_mon+1,cur_time.tm_mday,
            cur_time.tm_hour,cur_time.tm_min,cur_time.tm_sec);

    return len;
}
