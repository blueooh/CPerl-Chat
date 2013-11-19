all: chat chatd

chat:
	cd client;make

chatd:
	cd server;make

clean:
	cd client;make clean
	cd server;make clean
