all: chat chatd

chat:
	cd client;make

chatd:
	cd server;make

install:
	cd client;make install
	cd server;make install

clean:
	cd client;make clean
	cd server;make clean
