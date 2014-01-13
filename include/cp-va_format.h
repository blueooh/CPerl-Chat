#ifndef __CPVA_FORMAT_H__
#define __CPVA_FORMAT_H__

/* macro to format string with variable args, using dynamic buffer size */
#define cp_va_format(__format)                                     \
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
#endif
