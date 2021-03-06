//
// ups_server.h: UpsServer interface
//
// Yuchen Wang (wangyuchen229@gmail.com)
// This code is in the public domain
//
#ifndef UPS_SERVER_H
#define UPS_SERVER_H

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

// Database server. The constructor starts it listening on the given
// port with the given io_service.
//
class UpsServer {
 public:
  UpsServer(boost::asio::io_service& io_service, unsigned port);
  ~UpsServer();

 private:
  UpsServer();
  void start_accept();

  struct UpsServerImpl;
  boost::scoped_ptr<UpsServerImpl> d;
};

#endif /* UPS_SERVER_H */

