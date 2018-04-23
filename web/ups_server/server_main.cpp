//
// server_main.cpp: Main server implementation
//
// Yuchen Wang (wangyuchen229@gmail.com)
// This code is in the public domain
//
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include "ups_server.h"
#include "packedmessage.h"
#include "stringdb.pb.h"
#include "world_handler.h"
#define WORLD_PORT 2000

using namespace std;
namespace asio = boost::asio;

int main(int argc, const char* argv[])
{
    unsigned port = 23456;
    cout << "Serving on port " << port << endl;

    try {
      using boost::asio;
      io_service world_service;
      ip::tcp::endpoint world_ep( ip::address::from_string("127.0.0.1"), WORLD_PORT);
      ip::tcp::socket world_sock(world_service);
      world_sock.async_connect(ep, world_initer);
      world_service.run();
    } catch (std::exception & e) {
      cerr << e.what() << endl;
    }
    
    try {
        asio::io_service io_service;
        UpsServer server(io_service, port);
        io_service.run();
    } catch (std::exception& e) {
      cerr << e.what() << endl;
    }

    return 0;
}
