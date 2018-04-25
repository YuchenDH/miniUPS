# Makefile for asio_protobuf_samples
# Expects protobuf and boost to be installed 
#
PROTOBUF_CFLAGS = `pkg-config --cflags protobuf`
PROTOBUF_LFLAGS = `pkg-config --libs protobuf`
BOOST_LFLAGS = -lboost_system
PQXX_LFLAGS = -lpqxx -lpq

server_main: main.o ups.pb.o au.pb.o 
	g++ -o server_main main.o ups.pb.o au.pb.o\
		$(PROTOBUF_LFLAGS) $(BOOST_LFLAGS) $(PQXX_LFLAGS)

ups.pb.o: proto/ups.pb.cc
	g++ -c proto/ups.pb.cc $(PROTOBUF_CFLAGS)

au.pb.o: proto/au.pb.cc
	g++ -c proto/au.pb.cc $(PROTOBUF_CFLAGS)

HEADERS = $(wildcard *.h)

main.o:main.cpp packedmessage.h db.h ups_server.h proto/ups.pb.o proto/au.pb.h
	g++ -std=c++11 main.cpp -o main.o -lpq -lpqxx -lboost_system

%.o: %.cpp proto/ups.pb.o proto/au.pb.o ${HEADERS}
	g++ -std=c++11 $<
