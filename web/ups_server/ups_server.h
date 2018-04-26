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
#include <sys/wait.h>
#include <ctime>
#include <mutex>
#include <unordered_map>
#include "packedmessage.h"
#include "db.h"
#include "ups.pb.h"
#include "au.pb.h"
#include "config.h"
#define NUMTRUCKS 5
#define RECONNECTID 10
#define AMZ_ADDRESS "127.0.0.1"
#define AMZ_PORT 23457

using namespace std;
namespace asio = boost::asio;
//namespace au = proto::au;
//namespace ups = proto::ups;
using asio::ip::tcp;
using boost::uint8_t;

typedef std::vector<boost::uint8_t> data_buffer;

class UpsServer : public boost::enable_shared_from_this<UpsServer> {
 public:
  typedef boost::shared_ptr<UpsServer> Pointer;
  /*
  typedef boost::shared_ptr<ups::UCommandss> CommandsPointer;
  typedef boost::shared_ptr<ups::UResponse> ResponsesPointer;
  typedef boost::shared_ptr<au::A2U> A2UPointer;
  typedef boost::shared_ptr<au::U2A> U2APointer;
  */
  typedef boost::shared_ptr<ups::UCommands> UCpointer;
  typedef boost::shared_ptr<ups::UResponses> URpointer;
  typedef boost::shared_ptr<au::A2U> A2Upointer;
  typedef boost::shared_ptr<au::U2A> U2Apointer;

  static Pointer create(asio::io_service& world, asio::io_service& amz, db::dbPointer& database) {
    return Pointer(new UpsServer(world, amz, database));
  }
  
private:
  tcp::socket amz_sock;
  tcp::socket world_sock;
  db::dbPointer dblink;
  PackedMessage<ups::UCommands> packed_uc;
  PackedMessage<ups::UResponses> packed_ur;
  PackedMessage<au::A2U> packed_a2u;
  PackedMessage<au::U2A> packed_u2a;
  
  vector<uint8_t> amz_readbuf;
  data_buffer world_readbuf;
  std::mutex db_lock;

  std::unordered_map<int,time_t> truck_ready;
  bool truckshortage = false;

 UpsServer(asio::io_service& world, asio::io_service& amz, db::dbPointer& database) : amz_sock(amz), world_sock(world), dblink(database),
    packed_uc(boost::shared_ptr<ups::UCommands>(new ups::UCommands())),
    packed_ur(boost::shared_ptr<ups::UResponses>(new ups::UResponses())),
    packed_a2u(boost::shared_ptr<au::A2U>(new au::A2U())),
    packed_u2a(boost::shared_ptr<au::U2A>(new au::U2A())) {

  }
  long gen_package_id(long oreder_id){
    time_t nowtime;  
    nowtime = time(NULL); 
    char tmp[64];   
    strftime(tmp,sizeof(tmp),"%Y%m%d",localtime(&nowtime));
    std::string res(tmp); 
    res+=std::to_string(oreder_id);
    return std::stol(res);
  }
  void connect_world(){
    //handle connection to world with world_sock;
    tcp::endpoint world_ep(asio::ip::address::from_string("127.0.0.1"), 12345);
    world_sock.connect(world_ep);
    //send UConnect
    boost::shared_ptr<ups::UConnect> ucon(new ups::UConnect);
    ucon->set_reconnectid(RECONNECTID);
    ucon->set_numtrucksinit(NUMTRUCKS);
    //send();
    vector<uint8_t> writebuf;
    PackedMessage<ups::UConnect> ucon_msg(ucon);
    ucon_msg.pack(writebuf);
    asio::write(world_sock, asio::buffer(writebuf));
    DEBUG && cerr << "Sent UConnect\n";
    //recv UConnected
    vector<uint8_t> readbuf;
    readbuf.resize(HEADER_SIZE);
    asio::read(world_sock, asio::buffer(readbuf));
    DEBUG && cerr << "Got header\n";
    DEBUG && cerr << show_hex(readbuf) << endl;
    PackedMessage<ups::UConnected> ucond;
    unsigned msg_len = ucond.decode_header(readbuf);
    DEBUG && cerr << msg_len << " bytes\n";

    readbuf.resize(HEADER_SIZE + msg_len);
    asio::mutable_buffers_1 buf = asio::buffer(&readbuf[HEADER_SIZE], msg_len);
    asio::read(world_sock, buf);
    DEBUG && cerr << "Got UConnected\n";
    DEBUG && cerr << show_hex(readbuf) << endl;
    if (ucond.unpack(readbuf)) {
      boost::shared_ptr<ups::UConnected> msg = ucond.get_msg();
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
    tcp::endpoint amz_ep(asio::ip::address::from_string(AMZ_ADDRESS), AMZ_PORT);
    boost::system::error_code ec;
    do {
      amz_sock.connect(amz_ep, ec);
      if (ec) {
	cerr << "Error when connecting to amz server at " << AMZ_ADDRESS << ":" << AMZ_PORT << endl;
      }
    } while(ec != 0)
      
    DEBUG && cerr << "Connected to amz\n";
  }
  
  void world_handle_read_header(const boost::system::error_code& error) {
    if (!error) {
      DEBUG && (cerr << "Got header from world \n" << endl);
      DEBUG && (cerr << show_hex(world_readbuf) << endl);
      unsigned msg_len = packed_ur.decode_header(world_readbuf);
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
		     boost::bind(&UpsServer::world_handle_read_header,
				 shared_from_this(),
				 asio::placeholders::error));
  }

  void world_start_read_body(unsigned msg_len) {
    world_readbuf.resize(HEADER_SIZE + msg_len);
    asio::mutable_buffers_1 buf = asio::buffer(&world_readbuf[HEADER_SIZE], msg_len);
    asio::async_read(world_sock, buf, 
		     boost::bind(&UpsServer::world_handle_read_body,
				 shared_from_this(),
				 asio::placeholders::error));
  }

  void send_msg(tcp::socket& sock, vector<uint8_t> writebuf) {
    asio::async_write(sock, asio::buffer(writebuf),
		      boost::bind(&UpsServer::dummy, shared_from_this()));
  }

  inline void dummy() {}
  
  void world_handle_request() {
    //unpack world_readbuf
    if (packed_ur.unpack(world_readbuf)) {
      URpointer ures = packed_ur.get_msg();
      // ups::UCommands * temp = NULL;
      ups::UCommands test;
      UCpointer temp(test.New());
      U2Apointer resp = prepare_U2A(ures, temp);
      //assert resp is not empty (happens when world sent a "deliver_made"-only response)
///	
      if (temp != NULL && temp->pickups_size()) {
	vector<uint8_t> writebuf;
	UCpointer temppointer(temp);
	PackedMessage<ups::UCommands> resp_msg(temppointer);
	resp_msg.pack(writebuf);
	send_msg(world_sock, writebuf);
      }
      
      if (resp->gp_size() != 0 || resp->ta_size() != 0){
	//pack message and send to amz
	vector<uint8_t> writebuf;
	PackedMessage<au::U2A> resp_msg(resp);
	resp_msg.pack(writebuf);
	send_msg(amz_sock, writebuf);
      }
    }
  }
  U2Apointer prepare_U2A(URpointer ures, UCpointer temp){
    if(ures->has_error()){
      //has error, handle it
      cerr << "Error msg in world response: " << ures->error() << endl;
      return NULL;
    }
    else{
      //process UDelivaryMade
      for(int i=0 ;i<ures->delivered_size();++i){
	// int truck_id = ures->delivered(i).truckid();
        long package_id = ures->delivered(i).packageid();
        std::string ins("update search_orders set status=5 where tracking_num = ");
        ins+=std::to_string(package_id);ins+=";";
        dblink->update(ins);
        //update db based on pid(tracking num) and truckid
        //return U2A();//need thinking
      }
      U2Apointer response(new au::U2A());

      //process UFinished

      for(int i=0 ;i<ures->completions_size();++i){
        int truck_id = ures->completions(i).truckid();
        int status = dblink->get_truck_status(truck_id);//0:free/idle 1:ready 2:pickup 3:wait for loading 4:out of delivery
        int x = ures->completions(i).x();
        int y = ures->completions(i).y();
        if(status == 2){//2:pick up
          //arrive at warehouse, need to load
          
          //set truck info
          au::Truck * tr = new au::Truck();//need delete
          tr->set_id(truck_id);
          //tr->set_X(x);tr->set_Y(y);
          au::U2Atruckarrive * temp = response->add_ta(); 
          temp->set_allocated_tr(tr);

          //set wh info
          int whid = dblink->get_warehouse_id(x,y);
          temp->set_whid(whid);

          //set order info
          std::vector<long> * res = dblink->get_oid_by_truckid(truck_id);//res need delete 
          for(size_t i=0;i<res->size();++i){
            temp->set_oids(i,res->at(i));
          }

          //set U2Agenpid
          for(size_t i=0;i<res->size();++i){
            au::U2Agenpid * gp = response->add_gp();
            gp->set_oid(res->at(i));
            gp->set_pid(dblink->get_pid_by_oid(res->at(i)));
          }     
          delete res;
          std::string ins("update search_trucks set status = 3 where truck_id =");
          ins=ins + std::to_string(truck_id)+";";
          dblink->update(ins);                     
        }
        else if(status == 4){//4:delivered
          //finished delivery, truck is free now
          //update truck status and location
          std::string ins("update search_trucks set status = 0,xlocation=");
          ins+=std::to_string(x);
          ins+=",ylocation=";
          ins+=std::to_string(y);
          ins+=" where truck_id = ";
          ins+=std::to_string(truck_id);
          ins+=";";
          dblink->update(ins);
          if(truckshortage){
            assign_truck(temp);
          }
        }
        else{
          //something wrong happen
          
        }  
      }
      return response;    
    }
  }
  void freeU2A(U2Apointer response){
    //free memory allocated during prepare U2A
    for(int i=0;i<response->ta_size();++i){
      delete response->mutable_gp(i);
    }
    for(int i=0;i<response->gp_size();++i){
      //delete response->mutable_ta(i)->tr();
      delete response->mutable_ta(i);
    }
    //delete response;
  }
  void amz_handle_read_header(const boost::system::error_code& error) {
    if (!error) {
      DEBUG && (cerr << "Got header from amz\n" << endl);
      DEBUG && (cerr << show_hex(amz_readbuf) << endl);
      unsigned msg_len = packed_a2u.decode_header(amz_readbuf);
      DEBUG && (cerr << msg_len << " bytes\n");
      amz_start_read_body(msg_len);
    }
  }

  void amz_handle_read_body(const boost::system::error_code& error) {
    DEBUG && (cerr << "handle body from amz" << error << endl);
    if (!error) {
      DEBUG && (cerr << "Got body\n");
      DEBUG && (cerr << show_hex(amz_readbuf) << endl);
      
      amz_handle_request();
      amz_start_read_header();
    } else {
      cerr << "error in UpsServer::world_handle_read_body(): ec: " << error << endl;
    }
  }

  void amz_start_read_header() {
    amz_readbuf.resize(HEADER_SIZE);
    asio::async_read(amz_sock, asio::buffer(amz_readbuf),
		     boost::bind(&UpsServer::amz_handle_read_header,
				 shared_from_this(),
				 asio::placeholders::error));
  }

  void amz_start_read_body(unsigned msg_len) {
    amz_readbuf.resize(HEADER_SIZE + msg_len);
    asio::mutable_buffers_1 buf = asio::buffer(&amz_readbuf[HEADER_SIZE], msg_len);
    asio::async_read(amz_sock, buf, 
		     boost::bind(&UpsServer::amz_handle_read_body,
				 shared_from_this(),
				 asio::placeholders::error));
  }
  
  void amz_handle_request() {
    if (packed_a2u.unpack(amz_readbuf)) {
      A2Upointer ares = packed_a2u.get_msg();
      UCpointer resp = prepare_UCommands(ares);

      //pack message and send to amz
      vector<uint8_t> writebuf;
      PackedMessage<ups::UCommands> resp_msg(resp);
      resp_msg.pack(writebuf);
      send_msg(world_sock, writebuf);
    }
  }
  
  void assign_truck(UCpointer response){
    int truck_id = -1;
    while(dblink->has_unprocessed_order() && (truck_id = dblink->get_free_truck())>0){
      int whid = bind_order_with_truck(truck_id);
      ups::UGoPickup * temp = response->add_pickups();
      temp->set_truckid(truck_id);
      temp->set_whid(whid);      
    }
    if(dblink->has_unprocessed_order() && (truck_id = dblink->get_free_truck())<0){
      truckshortage=true;
    }
  }
  
  UCpointer prepare_UCommands(A2Upointer a2u){
    // ups::UCommands * response = new ups::UCommands();
    UCpointer response(new ups::UCommands());

    //process A2Upickuprequest
    for(int i=0;i<a2u->pr_size();++i){
      insert_order_to_dblink(a2u->mutable_pr(i));
    }
    assign_truck(response);

    //process A2Utruckdepart
    for(int i=0;i<a2u->td_size();++i){
      update_dblink_by_td(a2u->mutable_td(i),response);
    }
    return response;
  }

  int bind_order_with_truck(int truck_id){
    int whid = dblink->get_oldest_order_whid();
    std::vector<long> * temp = dblink->get_oid_by_whid(whid);//need delete
    for(size_t i=0;i<temp->size();++i){
      std::string ins("update search_orders set truck_id = ");
      ins+=std::to_string(truck_id);
      ins+=" where order_id = ";
      ins+=std::to_string(temp->at(i));
      ins+=";";
    }
    return whid;
  }

  void insert_order_to_dblink(au::A2Upickuprequest* pr){
    long order_id = pr->oid();
    long package_id = gen_package_id(order_id);
    int whid = pr->wh().id();
    //store warehouse info
    if(dblink->get_warehouse_id(pr->wh().x(),pr->wh().y())<0){
      //new warehouse info
      if(dblink->add_warehouse(whid,pr->wh().x(),pr->wh().y())<0){
        std::cout<<"add warehouse failed\r\n";
      }
    }

    int des_x = pr->destination().x();
    int des_y = pr->destination().y();
    int count = pr->items_size();
    std::string first_item(pr->items(0).des());
    if(pr->has_upsaccount()){
      int uid = dblink->get_uid_by_username(pr->upsaccount());
      if(uid<0){
        //not a valid user
        if(dblink->add_order(package_id,order_id,whid,des_x,des_y,1,-1,first_item,count)<0){
          std::cout<<"add order failed\r\n";
        }
      }
      if(dblink->add_order(package_id,order_id,whid,des_x,des_y,1,-1,uid,first_item,count)<0){
        std::cout<<"add order failed\r\n";
      }    
    }
    else{
      if(dblink->add_order(package_id,order_id,whid,des_x,des_y,1,-1,first_item,count)<0){
        std::cout<<"add order failed\r\n";
      }    
    }
    for(int i=0;i<pr->items_size();++i){
    dblink->add_item(pr->items(i).des(),pr->items(i).count(),order_id);
    }   
  }
  void update_dblink_by_td(au::A2Utruckdepart* td,UCpointer temp){
    //modify UCommand
    int truck_id = td->tr().id();
    ups::UGoDeliver * deliveries = temp->add_deliveries();
    deliveries->set_truckid(truck_id);
    std::vector<package*>* packages = dblink->get_package_by_truck(truck_id);
    for(size_t i=0;i<packages->size();++i){
      ups::UDeliveryLocation * package = deliveries->add_packages();
      package->set_packageid(packages->at(i)->package_id);
      package->set_x(packages->at(i)->x);
      package->set_y(packages->at(i)->y);
    }
    // update db
    std::string ins("update search_trucks set status = 4 where truck_id =");
    ins=ins+std::to_string(truck_id)+" ;";
    dblink->update(ins);

    std::string ins2("update search_orders set status = 4 where status=3 and truck_id =");
    ins=ins+std::to_string(truck_id)+" ;";
    dblink->update(ins);
  }

 public:
  void start() {
    connect_world();
    connect_amz();
    amz_start_read_header();
    world_start_read_header();
  }
  

};
