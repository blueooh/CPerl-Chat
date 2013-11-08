TARGET=lightchat
INCS=-I../liblist -I../liblist/include
OBJS=chat_main.o
CC=-g -c
LIB=-L../lib -lncursesw -lpthread -llist -lcdk
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	cd ../liblist;make;
	gcc $(INCS) -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $(INCS) $<

chat_main.o: chat_main.c chat_main.h motd.h

clean:
	rm -rf $(TARGET) $(OBJS)
	cd ../liblist/;make clean

install:
	cp ../lib/liblist.so /usr/lib
	cp $(TARGET) /usr/bin

uninstall:
	rm -rf /usr/lib/liblist.so
	rm -rf /usr/bin/$(TARGET)
