#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>
#include <cstdlib>
#include "ups.pb.h"
#include "au.pb.h"
#include "packedmessage.h"
#include "ups_server.h"
#include "config.h"

using namespace std;
namespace asio = boost::asio;

void thread(asio::io_service & s) {
  s.run();
}

int main(int argc, const char* argv[]) {
  db::dbPointer db_ptr = db::create("postgres", "postgres");
  if (db_ptr->connect_db()) {
    cerr << "Error when connecting to db\n";
    return 1;
  }
  std::cout<<"db connected\r\n";
  asio::io_service io[2];
  boost::shared_ptr<UpsServer> server_ptr = UpsServer::create(io[0], io[1], db_ptr);
  std::cout<<"ups server built\n";
  server_ptr->start();
  std::cout<<"start success\n";
  boost::thread t(boost::bind(&thread, boost::ref(io[0])));
  io[1].run();
  t.join();
  return 0;
}
