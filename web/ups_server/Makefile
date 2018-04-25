# Makefile for asio_protobuf_samples
# Expects protobuf and boost to be installed 
#
PROTOBUF_CFLAGS = `pkg-config --cflags protobuf`
PROTOBUF_LFLAGS = `pkg-config --libs protobuf`
PROTO_LFLAGS = -lprotobuf
BOOST_LFLAGS = -lboost_system -lboost_thread
PQXX_LFLAGS = -lpqxx -lpq

server_main: main.o ups.pb.o au.pb.o 
	g++ -o server_main main.o ups.pb.o au.pb.o\
		$(PROTOBUF_LFLAGS) $(BOOST_LFLAGS) $(PQXX_LFLAGS)

ups.pb.o: proto/ups.pb.cc
	g++ -c proto/ups.pb.cc $(PROTOBUF_CFLAGS)

au.pb.o: proto/au.pb.cc
	g++ -c proto/au.pb.cc $(PROTOBUF_CFLAGS)

HEADERS = $(wildcard *.h)

#main.o:main.cpp packedmessage.h db.h ups_server.h proto/ups.pb.h proto/au.pb.h
#	g++ -std=c++11 main.cpp -o main.o $

%.o: %.cpp ups.pb.o au.pb.o ${HEADERS}
	g++ -std=c++11 $<
server: main.cpp packedmessage.h db.h ups_server.h ups.pb.h au.pb.h ups.pb.cc au.pb.cc
	g++ -o server -std=c++11 -Wall -Werror -ggdb3 main.cpp au.pb.cc ups.pb.cc $(BOOST_LFLAGS) $(PQXX_LFLAGS) $(PROTO_LFLAGS)
#proxy: proxy.cpp proxy.h
#	g++  -o proxy -std=gnu++11 -Wall -Werror -ggdb3 proxy.cpp -lpthread
