TARGET=cperl-chat
INCS=-I../liblist -I../include -I/usr/include/libgtop-2.0 -I/usr/include/glib-2.0
OBJS=cperl-chat.o
CC=-g -c
LIB=-L../lib -lncursesw -lgtop-2.0 -lglib-2.0 -lpthread -llist
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	mkdir -p ../lib;
	cd ../liblist;make;
	gcc $(INCS) -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $(INCS) $<

cperl-chat.o: cperl-chat.c ../include/cperl-chat.h ../include/cp-common.h ../include/cp-motd.h

dep:
	gccmakedep $(INCS) $(SRCS)

clean:
	rm -rf $(TARGET) $(OBJS)
	cd ../liblist/;make clean

install:
	cp ../lib/liblist.so /usr/lib
	cp ../script/naver_rank /usr/bin/
	chmod 777 /usr/bin/naver_rank
	cp ../script/weather /usr/bin/
	chmod 777 /usr/bin/weather
	cp $(TARGET) /usr/bin

uninstall:
	rm -rf /usr/lib/liblist.so
	rm -rf /usr/bin/$(TARGET)
