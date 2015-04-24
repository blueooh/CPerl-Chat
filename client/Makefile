TARGET=cperl-chat
INCS=-I../include -I/usr/include/libgtop-2.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
OBJS=cperl-chat.o ../module/cp-list.o ../module/cp-log.o
CC=-g -c
LIB=-lncursesw -lgtop-2.0 -lglib-2.0 -lpthread
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	gcc $(INCS) -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $(INCS) $<

cperl-chat.o: cperl-chat.c ../include/cperl-chat.h ../include/cp-common.h ../include/cp-motd.h ../include/cp-version.h

dep:
	gccmakedep $(INCS) $(SRCS)

clean:
	rm -rf $(TARGET) $(OBJS)

install:
	cp ../script/daum_rank /usr/bin/
	chmod 777 /usr/bin/daum_rank
	cp ../script/weather /usr/bin/
	chmod 777 /usr/bin/weather
	cp $(TARGET) /usr/bin

uninstall:
	rm -rf /usr/bin/$(TARGET)
