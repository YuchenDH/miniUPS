#include "world_handler.h"
using namespace std;
namespace asio = boost::asio;
using asio::ip::tcp;
using boost::uint8_t;

class WorldConnection : public boost::enable_shared_from_this<WorldConnection> {
public:
  typedef boost::shared_ptr<WorldConnection> Pointer;
  typedef boost::shared_ptr<ups::UConnect> ConnectPointer;
  typedef boost::shared_ptr<ups::UConnected> ConnectedPointer;
  typedef boost::shared_ptr<ups::UCommands> CommandsPointer;
  typedef boost::shared_ptr<ups::UResponses> ResponsesPointer;

  static Pointer create (asio::io_service& io_service, pqxx::connection& C) {
    return Pointer(new WorldConnection(io_service, C));
  }

  tcp::socket& get_socket() {
    return m_socket;
  }

  void start() {
    start_read_header();
  }

private:
  tcp::socket m_socket;
  pqxx::connection& m_db_connection;
  vector<uint8_t> m_readbuf;

  WorldConnection(asio::io_service& io_service, pqxx::connection& C) : m_socket(io_service), m_db_ref(C) {}

  void handle_read_header(const boost::system::error_code& error) {
    DEBUG && (cerr << "handle read " << error.message() << endl);
    if (!error) {
      DUBUG && (cerr << "Got header\n");
      unsigned msg_len = decode_header(m_readbuf);
      DEBUG && (cerr << msg_len << " bytes\n");
      start_read_body(msg_len);
    }
  }

  void handle_read_body(const boost::system::error_code& error) {
    DEBUG && (cerr << "handle body " << error << endl);
    if (!error) {
      DEBUG && (cerr << "Got body\n");
      handle_response();
      start_read_header();
    }
  }

  void handle_response() {
    //new a message body
    //call unpack(m_readbuf)

    //deal with response
    //identify response type
    
  }
  
