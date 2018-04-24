#include "socket.h"
#include "proto/ups.pb.h"
int main(int argc, char ** argv){
	socket* s = new socket();
	int fd = s->create_socket_fd();
	if(s->connect_world(std::string(argv[1]),23456,fd)==-1){
		std::cout<<"connect fail\r\n";
	}
	ups::UConnect uc;
	uc.set_reconnectid(1000);
	if(!s->sendMesgTo(&uc,fd)){
		std::cout<<"send fail\r\n";
	}
	ups::UConnected ucd;
	if(!s->recvMesgFrom(&ucd,fd)){
		std::cout<<"recv fail\r\n";
	}
	std::cout<<"connected world is "<<ucd.worldid()<<"\r\n";
	return EXIT_SUCCESS;
}