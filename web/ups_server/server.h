#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <pqxx/pqxx>
#include "packedmessage.h"
#include "db.h"
using namespace std;
namespace asio = boost::asio;
using asio::ip::tcp;
using boost::uint8_t;
typedef std::vector<boost::uint8_t> data_buffer;

class UpsServer : public boost::enable_shared_from_this<UpsServer> {
 public:
  typedef boost::shared_ptr<UpsServer> Pointer;
  typedef boost::shared_ptr<ups:UCommand> UCommand;
  typedef boost::shared_ptr<ups::UResponses> UResponses;
  typedef boost::shared_ptr<au::A2U> A2U;
  typedef boost::shared_ptr<au::U2A> U2A;
  void start() {
    connect_world();
    //connect_db();
    start_read_amz();
    start_read_world();
  }

  static Pointer create(asio::io_service& world, asio::io_service& amz, const std::string database) {
    return Pointer(new UpsServer(world, amz, database));
  }

 private:
  tcp::socket amz_sock;
  tcp::socket world_sock;
  pqxx::connection C;
  vector<uint8_t> amz_readbuf;
  data_buffer world_readbuf;
  std::mutex db_lock;
  //PackedMessage::PackedMessage amz_request;
  //PackedMessage::PackedMessage world_request;
  tcp::acceptor amz_acceptor;
  //PackedMessage::PackedMessage<au> amz_response;
  //PackedMessage::PackedMessage<world> world_response;
  PackedMessage<ups::UCommand> packed_uc;
  PackedMessage<ups::UResponses> packed_ur;
  PackedMessage<au::A2U> packed_a2u;
  PackedMessage<au::U2A> packed_u2a;

  UpsServer(asio::io_service& world, asio::io_service& amz, const std::string database) : amz_sock(amz), world_sock(world) {
    //connect to database
  }

  void connect_world(){
    //handle connection to world with world_sock;
  }

  void world_handle_read_header(const boost::system::error_code* error) {
    if (!error) {
      //unsigned msg_len = decode_header(readbuf);
    }
    world_start_read_body(msg_len);
  }

  void world_handle_read_body(const boost::system::error_code& error) {
    if (!error) {
      world_handle_request();
      world_start_read_header();
    } else {
      cerr << "error in UpsServer::world_handle_read_body(): ec: " << error << endl;
    }
  }

  void world_start_read_header() {
    world_readbuf.resize(HEADER_SIZE);
    asio::async_read(world_sock, asio::buffer(world_readbuf),
		     boost::bind(world_handle_read_header,
				 shared_from_this(),
				 asio::placeholders::error));
  }

  void world_start_read_body(unsigned msg_len) {
    world_readbuf.resize(HEADER_SIZE + msg_len);
    asio::mutable_buffers_1 buf = asio::buffer(&world_buffer[HEADER_SIZE], msg_len);
    asio::async_read(world_sock, buf, 
		     boost::bind(&UpsServer::world_handle_read_body,
				 shared_from_this(),
				 asio::placeholders::error));
  }
  

  void world_handle_request() {
    //unpack world_readbuf
    if (packed_ur.unpack(world_readbuf)) {
        UResponses ures = packed_ur.get_msg();
        if(ures->has_error()){
          //has error, handle it
        }
        else{
          for(int i=0 ;i<ures->delivered_size();++i){
            int truck_id = ures->delivered(i)
            long truck_id = ures->delivered(i)
          }
          for(int i=0 ;i<ures->completions_size();++i){
            
          }
        }
    }    

    //handle request
  }

  void amz_handle_read_header(const boost::system::error_code* error) {
    if (!error) {
      //unsigned msg_len = decode_header(readbuf);
    }
    amz_start_read_body(msg_len);
  }

  void amz_handle_read_body(const boost::system::error_code& error) {
    if (!error) {
      amz_handle_request(readbuf);
      amz_start_read_header();
    } else {
      cerr << "error in UpsServer::world_handle_read_body(): ec: " << error << endl;
    }
  }

  void amz_start_read_header() {
    amz_readbuf.resize(HEADER_SIZE);
    asio::async_read(amz_sock, asio::buffer(amz_readbuf),
		     boost::bind(amz_handle_read_header,
				 shared_from_this(),
				 asio::placeholders::error));
  }

  void amz_start_read_body(unsigned msg_len) {
    amz_readbuf.resize(HEADER_SIZE + msg_len);
    asio::mutable_buffers_1 buf = asio::buffer(&amz_buffer[HEADER_SIZE], msg_len);
    asio::async_read(amz_sock, buf, 
		     boost::bind(amz_handle_read_body,
				 shared_from_this(),
				 asio::placeholders::error));
  }
  
  void amz_handle_request() {
    //unpack amz_readbuf

    //handle request
  }
 
  void amz_start() {
    amz_acceptor.acceptor(amz_sock, tcp::endpoint(tcp::v4(), PORT));
    amz_start_accept();
  }

  void amz_start_accept() {
    amz_acceptor.async_accept(amz_sock, boost::bind(handle_accept, shared_from_this(), asio:;placeholders::error));
  }

  void handle_accept(const boost::system::error_code& error) {
    if(!error) {
      amz_start_read_header();
    }
    amz_start_accept();
  }

  void amz_send_response(){}
}
