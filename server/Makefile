TARGET=server
OBJS=server.o
CC=-g -c
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	gcc -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $<

chat_main.o: server.c

clean:
	rm -rf $(TARGET) $(OBJS)
