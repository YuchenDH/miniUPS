LIBS=-lpthread -lprotobuf
CC=g++
ARGS=-std=gnu++11 -Wall -Werror -ggdb3
CFLAGS=-std=gnu++11 -Wall -Werror -ggdb3 -O3
EXTRAFLAGS=-lpqxx -lpq
socket:ups.pb.o
	$(CC) $^ $(LIBS) -o $@
ups.pb.o: proto/ups.pb.cc proto/ups.pb.h
	$(CC) $(ARGS) -I/usr/include/google/protobuf proto/$*.cc -o $@
#server: server.cpp server.h
#	g++  -o server -std=gnu++11 -Wall -Werror -ggdb3 server.cpp -lpthread
#socket: socket.cpp socket.h
#	g++  -o socket -std=gnu++11 -Wall -Werror -ggdb3 socket.cpp -lpthread
db: db.cpp db.h
	$(CC) $(CFLAGS) -o db db.cpp $(EXTRAFLAGS)