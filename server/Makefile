TARGET=cperl-chatd
INCS=-I../liblist -I../include
OBJS=cp-server.o cp-log.o
CC=-g -c
LIB=-L../lib -llist
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	mkdir -p ../lib;
	cd ../liblist;make;
	gcc $(INCS) -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $(INCS) $<

cp-server.o: cp-server.c ../include/cp-server.h ../include/cp-common.h
cp-log.o: cp-log.c ../include/cp-log.h

clean:
	rm -rf $(TARGET) $(OBJS)
	cd ../liblist/;make clean

install:
	cp ../lib/liblist.so /usr/lib
	cp $(TARGET) /usr/bin

uninstall:
	rm -rf /usr/lib/liblist.so
	rm -rf /usr/bin/$(TARGET)
