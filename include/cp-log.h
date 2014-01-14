#ifndef __CP_LOG_H__
#define __CP_LOG_H__

#include <cp-va_format.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>

#define CP_LOG_BUFFER   1024

int cp_init_log(const char * file);
int cp_log(char *buf, ...);
int cp_get_log_time(char *buf, int size);
#endif
