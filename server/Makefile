TARGET=cperl-chatd
INCS=-I../include
OBJS=cp-server.o ../module/cp-list.o ../module/cp-log.o
CC=-g -c
LIB=
SRCS=$(OBJ:.o=.c)

$(TARGET): $(OBJS)
	gcc $(INCS) -o $@ $^ $(LIB)

.c.o:
	gcc $(CC) $(INCS) $<

cp-server.o: cp-server.c ../include/cp-server.h ../include/cp-common.h

clean:
	rm -rf $(TARGET) $(OBJS)

install:
	cp $(TARGET) /usr/bin

uninstall:
	rm -rf /usr/bin/$(TARGET)
