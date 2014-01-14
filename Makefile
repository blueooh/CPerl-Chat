all: chat chatd

chat:
	cd module;make
	cd client;make

chatd:
	cd module;make
	cd server;make

install:
	cd client;make install
	cd server;make install

clean:
	cd module;make clean
	cd client;make clean
	cd server;make clean
