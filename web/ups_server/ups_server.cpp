//
// ups_server.cpp: UpsServer implementation
//
// Yuchen Wang (wangyuchen229@gmail.com)
// This code is in the public domain
//
#include "config.h"
#include "ups_server.h"
#include "packedmessage.h"
#include "au_proto.pb.h"
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

using namespace std;
namespace asio = boost::asio;
using asio::ip::tcp;
using boost::uint8_t;

// Database connection - handles a connection with a single client.
// Create only through the UpsConnection::create factory.
//
class UpsConnection : public boost::enable_shared_from_this<UpsConnection> {
public:
  typedef boost::shared_ptr<UpsConnection> Pointer;
  typedef boost::shared_ptr<au_proto::A2U> A2UPointer;
  typedef boost::shared_ptr<au_proto::U2A> U2APointer;
  
  static Pointer create(asio::io_service& world_service, asio::io_service& amz_service, pqxx::connection& C) {
    return Pointer(new UpsConnection(world_service, amz_service, C));
  }
  
  tcp::socket& get_world_socket() {
    return m_world_socket;
  }

  tcp::socket& get_amz_socket() {
    return m_amz_socket;
  }

  void start() {
    start_read_header();
  }

private:
  tcp::socket m_socket;
  pqxx::connection& m_db_connection;
  vector<uint8_t> m_readbuf;
  PackedMessage<au_proto::Request> m_packed_request;

  UpsConnection(asio::io_service& io_service, pqxx::connection& C)
    : m_socket(io_service), m_db_ref(C),
      m_packed_request(boost::shared_ptr<au_proto::Request>(new au_proto::Request())) {
  }
    
  void handle_read_header(const boost::system::error_code& error) {
    DEBUG && (cerr << "handle read " << error.message() << '\n');
    if (!error) {
      DEBUG && (cerr << "Got header!\n");
      unsigned msg_len = m_packed_request.decode_header(m_readbuf);
      DEBUG && (cerr << msg_len << " bytes\n");
      start_read_body(msg_len);
    }
  }

  void handle_read_body(const boost::system::error_code& error) {
    DEBUG && (cerr << "handle body " << error << '\n');
    if (!error) {
      DEBUG && (cerr << "Got body!\n");
      DEBUG && (cerr << show_hex(m_readbuf) << endl);
      handle_request();
      start_read_header();
    }
  }

  // Called when enough data was read into m_readbuf for a complete request
  // message. 
  // Parse the request, execute it and send back a response.
  //
  void handle_request() {
    if (m_packed_request.unpack(m_readbuf)) {
      RequestPointer req = m_packed_request.get_msg();
      ResponsePointer resp = prepare_response(req);
      
      vector<uint8_t> writebuf;
      PackedMessage<au_proto::Response> resp_msg(resp);
      resp_msg.pack(writebuf);
      asio::write(m_socket, asio::buffer(writebuf));
    }
  }

  void start_read_header() {
    m_readbuf.resize(HEADER_SIZE);
    asio::async_read(m_socket, asio::buffer(m_readbuf),
		     boost::bind(&UpsConnection::handle_read_header, shared_from_this(),
				 asio::placeholders::error));
  }

  void start_read_body(unsigned msg_len) {
    // m_readbuf already contains the header in its first HEADER_SIZE
    // bytes. Expand it to fit in the body as well, and start async
    // read into the body.
    //
    m_readbuf.resize(HEADER_SIZE + msg_len);
    asio::mutable_buffers_1 buf = asio::buffer(&m_readbuf[HEADER_SIZE], msg_len);
    asio::async_read(m_socket, buf,
		     boost::bind(&UpsConnection::handle_read_body, shared_from_this(),
				 asio::placeholders::error));
  }

  ResponsePointer prepare_response(RequestPointer req) {
    //need rewriting

  }
};


struct UpsServer::UpsServerImpl {
  tcp::acceptor acceptor;
  pqxx::connection C;

  UpsServerImpl(asio::io_service& io_service, unsigned port)
    : acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
    //connect to postgres
    start_accept();
  }

  void start_accept() {
    // Create a new connection to handle a client. Passing a reference
    // to db to each connection poses no problem since the server is 
    // single-threaded.

    UpsConnection::Pointer new_connection = 
      UpsConnection::create(acceptor.io_service(), C);

    // Asynchronously wait to accept a new client
    //
    acceptor.async_accept(new_connection->get_socket(),
			  boost::bind(&UpsServerImpl::handle_accept, this, new_connection,
				      asio::placeholders::error));
  }

  void handle_accept(UpsConnection::Pointer connection,
		     const boost::system::error_code& error) {
    // A new client has connected
    //
    if (!error) {
      // Start the connection
      //
      connection->start();

      // Accept another client
      //
      start_accept();
    }
  }
};


UpsServer::UpsServer(asio::io_service& io_service, unsigned port) : d(new DbServerImpl(io_service, port)) {
}


UpsServer::~UpsServer() {
}


