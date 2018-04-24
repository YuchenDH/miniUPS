#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <cstdlib>
#include "ups.pb.h"
#include "au.pb.h"
#include "packedmessage.h"
#include "ups_server.cpp"


using namespace std;
namespace asio = boost::asio;

void thread(asio::io_service & s) {
  s.run();
}

int main(int argc, const char* argv[]) {
  db::dbPointer db_ptr = db::create("upsweb", "postgres", "950703");
  if (db_ptr->connect_db()) {
    cerr << "Error when connecting to db\n";
    return 1;
  }
  
  asio::io_service io[2];
  boost::shared_ptr<UpsServer> server_ptr = UpsServer::create(io[0], io[1], db_ptr);
  server_ptr->start();
  
  boost::thread t{thread, io[0]};
  io[1].run();
  t.joint();
  return 0;
}
