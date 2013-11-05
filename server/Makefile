TARGET=bluechatd
INCS=-I../liblist
OBJS=server.o
CC=-g -c
LIB=-L../lib -llist
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	cd ../liblist;make;
	gcc $(INCS) -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $(INCS) $<

server.o: server.c server.h

clean:
	rm -rf $(TARGET) $(OBJS)
	cd ../liblist/;make clean

install:
	cp ../lib/liblist.so /usr/lib
	cp $(TARGET) /usr/bin

uninstall:
	rm -rf /usr/lib/liblist.so
	rm -rf /usr/bin/$(TARGET)
