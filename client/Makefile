TARGET=bluechat
OBJS=chat_main.o list.o
CC=-g -c
LIB=-lncursesw -lpthread
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	gcc -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $<

chat_main.o: chat_main.c chat_main.h motd.h list.h
list.o: list.c list.h

clean:
	rm -rf $(TARGET) $(OBJS)
