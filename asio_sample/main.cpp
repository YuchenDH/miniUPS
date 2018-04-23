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
#include "db.h"
#include "packedmessage.h"
#include "ups.pb.h"
#include "au.pb.h"

#define NUMTRUCKS 5
#define RECONNECTID 10
#define AMZ_ADDRESS "127.0.0.1"
#define AMZ_PORT 23457

using namespace std;
namespace asio = boost::asio;
using asio::ip::tcp;
using boost::uint8_t;

class UpsServer : public boost::enable_shared_from_this<UpsServer> {
public:
  typedef boost::shared_ptr<UpsServer> Pointer;
  typedef boost::shared_ptr<ups::UCommands> CommandsPointer;
  typedef boost::shared_ptr<ups::UResponse> ResponsesPointer;
  typedef boost::shared_ptr<au::A2U> A2UPointer;
  typedef boost::shared_ptr<au::U2A> U2APointer;
  
  void start() {
    connect_world();
    connect_amz();
    start_read_amz();
    start_read_world();
  }
  
  static Pointer create(asio::io_service& world, asio::io_service& amz, db::dbPointer database) {
    return Pointer(new UpsServer(world, amz, database));
  }
  
private:
  tcp::socket amz_sock;
  tcp::socket world_sock;
  db::dbPointer db;
  vector<uint8_t> amz_readbuf;
  vector<uint8_t> world_readbuf;
  std::mutex db_lock;
  PackedMessage<au::A2U> a2u_request;
  PackedMessage<au::U2A> u2a_response;
  tcp::acceptor amz_acceptor;
  PackedMessage<ups::UCommands> world_commands;
  PackedMessage<ups::UResponses> world_responses;
  
  UpsServer(asio::io_service& world, asio::io_service& amz, db::dbPointer database) : amz_sock(amz), world_sock(world), db(database) {
  }
  
  void connect_world(){
    //handle connection to world with world_sock;
    v4::endpoint world_ep(asio::ip::address::from_string("127.0.0.1"), 12345);
    world_sock.connect(world_ep);
    //send UConnect
    using ups;
    using PackedMessage;
    boost::shared_ptr<UConnect> ucon(new UConnect);
    ucon->set_reconnectid(RECONNECTID);
    ucon->set_numtrucksinit(NUMTRUCKS);
    //send();
    vector<uint_8> writebuf;
    PackedMessage<UConnect> ucon_msg(ucon);
    ucon_msg.pack(writebuf);
    asio::write(world_sock, aso::buffer(writebuf));
    DEBUG && cerr << "Sent UConnect\n"
    //recv UConnected
    vector<uint_8> readbuf;
    readbuf.resize(HEADER_SIZE);
    asio::read(world_sock, asio::buffer(readbuf));
    DEBUG && cerr << "Got header\n";
    DEBUG && cerr << show_hex(readbuf) << endl;
    PackedMessage<UConnected> ucond;
    unsigned msg_len = ucond.decode_header(readbuf);
    DEBUG && cerr << msg_len << " bytes\n";

    readbuf.resize(HEADER_SIZE + msg_len);
    asio::mutable_buffers_1 buf = asio::buffer(&readbuf[HEADER_SIZE], msg_len);
    asio::read(world_sock, buf);
    DEBUG && cerr << "Got UConnected\n";
    DEBUG && cerr << show_hex(readbuf) << endl;
    if (ucond.unpack(read_buf)) {
      boost::shared_ptr<UConnected> msg = ucond.get_msg();
      if (msg->has_error()) {
	cerr << msg->error() << endl;
	exit(EXIT_FAILURE);
      }
    } else {
      cerr << "Unable to decode UConnected message, exiting now...\n";
    }
  }
  
  void connect_amz() {
    //connect to amazon server
    v4::endpoint amz_ep(asio::ip::address::from_string(AMZ_ADDRESS, AMZ_PORT));
    boost::system::error_code ec;
    amz_sock.connect(amz_ep, ec);
    if (ec) {
      cerr << "Error when connecting to amz server at " << AMZ_ADDRESS << ":" << AMZ_PORT << endl;
    }
    DEBUG && "Connected to amz\n";
  }
  
  void world_handle_read_header(const boost::system::error_code* error) {
    if (!error) {
      DEBUG && (cerr << "Got header from world \n" << endl);
      DEBUG && (cerr << show_hex(world_readbuf) << endl);
      unsigned msg_len = world_responses.decode_header(world_readbuf);
      DEBUG && (cerr << msg_len << " bytes\n");
      world_start_read_body(msg_len);
    }
  }

  void world_handle_read_body(const boost::system::error_code& error) {
    DEBUG && (cerr << "handle body from world " << error << endl);
    if (!error) {
      DEBUG && (cerr << "Got body\n");
      DEBUG && (cerr << show_hex(world_readbuf) << endl);
      world_handle_request();
      world_start_read_header();
    } else {
      cerr << "error in UpsServer::world_handle_read_body(): ec: " << error << endl;
    }
  }

  void world_start_read_header() {
    world_readbuf.resize(HEADER_SIZE);
    asio::async_read(world_sock, asio::buffer(world_readbuf),
		     boost::bind(&world_handle_read_header,
				 shared_from_this(),
				 asio::placeholders::error));
  }

  void world_start_read_body(unsigned msg_len) {
    world_readbuf.resize(HEADER_SIZE + msg_len);
    asio::mutable_buffers_1 buf = asio::buffer(&world_buffer[HEADER_SIZE], msg_len);
    asio::async_read(world_sock, buf, 
		       boost::bind(&world_handle_read_body,
				 shared_from_this(),
				 asio::placeholders::error));
  }

  void send_msg(tcp::socket sock, vector<uint8_t> writebuf) {
    asio::async_write(sock, asio::buffer(writebuf),
		      boost::bind(&dummy, shared_from_this()));
  }

  inline void dummy() {}
  void world_handle_request() {
    //unpack world_readbuf

    //handle request
  }

  void amz_handle_read_header(const boost::system::error_code* error) {
    if (!error) {
      DEBUG && (cerr << "Got header from amz\n" << endl);
      DEBUG && (cerr << show_hex(amz_readbuf) << endl);
      unsigned msg_len = a2z_request.decode_header(world_readbuf);
      DEBUG && (cerr << msg_len << " bytes\n");
      amz_start_read_body(msg_len);
    }
  }

  void amz_handle_read_body(const boost::system::error_code& error) {
    DEBUG && (cerr << "handle body from amz" << error << endl);
    if (!error) {
      DEBUG && (cerr << "Got body\n");
      DEBUG && (cerr << show_hex(amz_readbuf) << endl);
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
 
}
