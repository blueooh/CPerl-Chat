TARGET=bluechatd
OBJS=server.o
CC=-g -c
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	gcc -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $<

chat_main.o: server.c server.h

clean:
	rm -rf $(TARGET) $(OBJS)
