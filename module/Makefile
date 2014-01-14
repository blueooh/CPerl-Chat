INCS=-I../include
CC=-g -c
OBJS=cp-list.o cp-log.o

all: cp-list cp-log

cp-list: 
	gcc $(CC) $(INCS) cp-list.c

cp-log: 
	gcc $(CC) $(INCS) cp-log.c

clean:
	rm -rf $(OBJS) 
