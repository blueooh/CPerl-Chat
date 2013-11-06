CC=gcc
CFLAGS= -c -Wall -p -fPIC
LDFLAGS= -shared -o liblist.so
OBJS= list.o

all: list

.c.o:
	$(CC) $(CFLAGS) $? 

list: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS)

clean:  
	rm -f *.o
	rm -f list liblist.so
