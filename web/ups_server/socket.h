#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

class socket{
private:
	std::string hostname;
	int port;
	int host_fd;
public:
	socket(){
		
	}
	socket(int portnum):port(portnum){
		char host[64];
		gethostname(host,64);
		hostname = std::string(host);
		host_fd = socket(AF_INET,SOCK_STREAM,0);
	}
	socket(const socket &rhs){
		host_fd = rhs.host_fd;
		port = rhs.port;
		hostname = rhs.hostname;
	}
	~socket(){
	  close(hostfd);
	}
	int create_socket_fd(){
		int socket_fd = socket(AF_INET,SOCK_STREAM,0);
		return socket_fd;	
	}
	void close_socket_fd(int socket_fd){
		close(socket_fd);
	}
	int connect_world(std::string hostname,int port,int socket_fd){
	  	struct sockaddr_in server_in;
	  	//memset(server_in,0,sizeof(server_in));
  		server_in.sin_family = AF_INET;
  		server_in.sin_port = htons(port);
  		
  		struct hostent * host_info = gethostbyname(hostname.c_str());
  		if ( host_info == NULL ) {
  			std::cout<<hostname<<" host info is null"<<std::endl;
    		return -1;
  		}
		memcpy(&server_in.sin_addr, host_info->h_addr_list[0], host_info->h_length);
		int connect_status = connect(socket_fd,(struct sockaddr *)&server_in,sizeof(server_in));
		return connect_status;
	}
	void bind_addr(){
		struct hostent * host_info = gethostbyname(hostname.c_str());
		struct sockaddr_in server_in;
  		server_in.sin_family = AF_INET;
  		server_in.sin_port = htons(port);
		memcpy(&server_in.sin_addr, host_info->h_addr_list[0], host_info->h_length);
		int bind_status = bind(host_fd,(struct sockaddr *)&server_in,sizeof(server_in));
		if(bind_status<0){
			std::cout<<"bind fail"<<std::endl;
		}
		listen(host_socket_fd,5);
	}
	template<typename T>
	bool sendMesgTo(const T & message,int fd) {
		//extra scope: make output go away before out->Flush()
		// We create a new coded stream for each message. Don’t worry, this is fast.
		google::protobuf::io::FileOutputStream * out = new google::protobuf::io::FileOutputStream(fd);
		google::protobuf::io::CodedOutputStream output(out);
		// Write the size.
		const int size = message.ByteSize();
		output.WriteVarint32(size);
		uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
		if (buffer != NULL) {
		// Optimization: The message fits in one buffer, so use the faster
		// direct-to-array serialization path.
		message.SerializeWithCachedSizesToArray(buffer);
		} 
		else{
			// Slightly-slower path when the message is multiple buffers.
			message.SerializeWithCachedSizes(&output);
		}
		if (output.HadError()) {
			delete out;
			return false;
		}
		out->Flush();
		delete out;
		return true;
	}
	template<typename T>
	bool recvMesgFrom(T & message,int fd){
		google::protobuf::io::FileOutputStream * in = new google::protobuf::io::FileInputStream(fd);
		google::protobuf::io::CodedInputStream input(in);
		uint32_t size;
		if (!input.ReadVarint32(&size)) {
			delete in;
			return false;
		}
		// Tell the stream not to read beyond that size.
		google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);
		// Parse the message.
		if (!message.MergeFromCodedStream(&input)) {
			delete in;
			return false;
		}
		if (!input.ConsumedEntireMessage()) {
			delete in;
			return false;
		}
		// Release the limit.
		input.PopLimit(limit);
		delete in;
		return true;
	}
}