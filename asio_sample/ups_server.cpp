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
#include <ctime>
#include <unordered_map>
#include "packedmessage.h"
#include "db.h"
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
  typedef boost::shared_ptr<ups:UCommandss> UCommandss;
  typedef boost::shared_ptr<ups::UResponses> UResponses;
  typedef boost::shared_ptr<au::A2U> A2U;
  typedef boost::shared_ptr<au::U2A> U2A;

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
  data_buffer world_readbuf;
  std::mutex db_lock;

  PackedMessage<ups::UCommands> packed_uc;
  PackedMessage<ups::UResponses> packed_ur;
  PackedMessage<au::A2U> packed_a2u;
  PackedMessage<au::U2A> packed_u2a;
  
  std::unordered_map<int,time_t> truck_ready;
  bool truckshortage = false;
  UpsServer(asio::io_service& world, asio::io_service& amz, db::dbPointer database) : amz_sock(amz), world_sock(world), db(database) {
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
    if (packed_ur.unpack(world_readbuf)) {
        UResponses ures = packed_ur.get_msg();
        ups::UCommands * temp = NULL;
        U2A resp = prepare_U2A(ures,temp);
	      //assert resp is not empty (happens when world sent a "deliver_made"-only response)

	
	      //pack message and send to amz
	      vector<uint8_t> writebuf;
	      PackedMessage<au::U2A> resp_msg(resp);
	      resp_msg.pack(writebuf);
	      send_msg(amz_sock, writebuf);
    }
  }
  U2A prepare_U2A(UResponse ures,ups::UCommands * temp){
    if(ures->has_error()){
      //has error, handle it
      cerr << "Error msg in world response: " << ures->error() << endl;
    }
    else{
      //process UDelivaryMade
      for(int i=0 ;i<ures->delivered_size();++i){
        int truck_id = ures->delivered(i)->truckid();
        long package_id = ures->delivered(i)->packageid();
        std::string ins("update search_orders set status=5 where tracking_num = ");
        ins+=std::to_string(package_id);ins+=";";
        db->update(ins);
        //update db based on pid(tracking num) and truckid
        return U2A();//need thinking
      }
      U2A response(new au::U2A());

      //process UFinished

      for(int i=0 ;i<ures->completions_size();++i){
        int truck_id = ures->completions(i)->truckid();
        int status = db->get_truck_status(truck_id);//0:free/idle 1:ready 2:pickup 3:wait for loading 4:out of delivery
        int x = ures->completions(i)->x();
        int y = ures->completions(i)->y();
        if(status == 2){//2:pick up
          //arrive at warehouse, need to load
          
          //set truck info
          au::Truck * tr = new au::Truck();//need delete
          tr->set_id(truck_id);
          //tr->set_X(x);tr->set_Y(y);
          au::U2Atruckarrive * temp = response->add_ta(); 
          temp->set_allocated_truck(tr);

          //set wh info
          int whid = db->get_warehouse_id(x,y);
          temp->set_whid(whid);

          //set order info
          std::vector<long> * res = db->get_oid_by_truckid(truck_id);//res need delete 
          for(int i=0;i<res->size();++i){
            temp->set_oids(i,res->at(i));
          }

          //set U2Agenpid
          for(int i=0;i<res->size();++i){
            au::U2Agenpid * gp = response->add_gp();
            gp->set_oid(res->at(i));
            gp->set_pid(db->get_pid_by_oid(res->at(i)));
          }     
          delete res;
          std::string ins("update search_trucks set status = 3 where truck_id =");
          ins=ins + std::to_string(truck_id)+";";
          db->update(ins);                     
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
          db->update(ins);
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
  void freeU2A(U2A response){
    //free memory allocated during prepare U2A
    for(int i=0;i<response->ta_size();++i){
      delete response->gp(i);
    }
    for(int i=0;i<response->gp_size();++i){
      delete response->ta(i)->tr();
      delete response->ta(i);
    }
    delete response;
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
    if (packed_a2u.unpack(amz_readbuf)) {
        A2U ares = packed_a2u.get_msg();
        UCommandss resp = prepare_UCommandss(ares);

	     //pack message and send to amz
	      vector<uint8_t> writebuf;
	      PackedMessage<au::UCommandss> resp_msg(resp);
	      resp_msg.pack(writebuf);
	      send_msg(world_sock, writebuf);
    }
  }

  void assign_truck(ups::UCommands * response){
    int truck_id = -1;
    while(db->has_unprocessed_order() && (truck_id = db->get_free_truck())>0){
      int whid = bind_order_with_truck(truck_id);
      ups::UGoPickup * temp = response->add_pickups();
      temp->set_truckid(truck_id);
      temp->set_whid(whid);      
    }
    if(db->has_unprocessed_order() && (truck_id = db->get_free_truck())<0){
      truckshortage=true;
    }
  }
  UCommands prepare_UCommands(A2U a2u){
    ups::UCommands * response = new ups::UCommands();

    //process A2Upickuprequest
    for(int i=0;i<a2u->pr_size();++i){
      insert_order_to_db(a2u->mutable_pr(i));
    }
    assign_truck(response);

    //process A2Utruckdepart
    for(int i=0;i<a2u->td_size();++i){
      update_db_by_td(a2u->mutable_td(i));
    } 
  }

  int bind_order_with_truck(int truck_id){
    int whid = db->get_oldest_order_whid();
    std::vector<long> * temp = db->get_oid_by_whid(whid);//need delete
    for(int i=0;i<temp->size();++i){
      std::string ins("update search_orders set truck_id = ");
      ins+=std::to_string(truck_id);
      ins+=" where order_id = ";
      ins+=std::to_string(temp->at(i));
      ins+=";";
    }
    return whid;
  }

  void insert_order_to_db(au::A2Upickuprequest* pr){
    long order_id = pr->oid();
    long package_id = gen_package_id(oreder_id);
    int whid = pr->wh()->id();
    //store warehouse info
    if(db->get_warehouse_id(pr->wh()->X(),pr->wh()->Y())<0){
      //new warehouse info
      if(db->add_warehouse(whid,pr->wh()->X(),pr->wh()->Y())<0){
        std::cout<<"add warehouse failed\r\n";
      }      
    }

    int des_x = pr->destination()->X();
    int des_y = pr->destination()->Y();
    if(pr->has_upsaccount()){
      int uid = db->get_uid_by_username(pr->upsaccount());
      if(uid<0){
        //not a valid user
        if(db->add_order(package_id,order_id,whid,des_x,des_y,1,-1)<0){
          std::cout<<"add order failed\r\n";
        }
      }
      if(db->add_order(package_id,order_id,whid,des_x,des_y,1,-1,uid)<0){
        std::cout<<"add order failed\r\n";
      }    
    }
    else{
      if(db->add_order(package_id,order_id,whid,des_x,des_y,1,-1)<0){
        std::cout<<"add order failed\r\n";
      }    
    }
    for(int i=0;i<pr->items_size();++i){
    db->add_item(pr->items(i)->des(),pr->items(i)->count(),order_id);
    }   
  }
  void update_db_by_td(au::A2Utruckdepart* td,ups::UCommands * temp){
    //modify UCommand
    int truck_id = td->tr()->id();
    ups::UGoDeliver deliveries = temp->add_deliveries();
    deliveries->set_truckid(truck_id);
    std::vector<db::package*>* packages = db->get_package_by_truck();
    for(int i=0;i<packages->size();++i){
      ups::UDelivaryLocation package = deliveries->add_packages();
      package->set_packageid(packages->at(i)->package_id);
      package->set_x(packages->at(i)->x);
      package->set_y(packages->at(i)->y);
    }
    // update db
    std::string ins("update search_trucks set status = 4 where truck_id =");
    ins=ins+std::to_string(truck_id)+" ;";
    db->update(ins);

    std::string ins2("update search_orders set status = 4 where status=3 and truck_id =");
    ins=ins+std::to_string(truck_id)+" ;";
    db->update(ins);
  }

  // void update_db_by_pr(au::A2Upickuprequest* pr){
  //   long oreder_id = pr->oid();
  //   long package_id = gen_package_id(oreder_id);
  //   int whid = pr->wh()->id();
  //   int des_x = pr->destination()->X();
  //   int des_y = pr->destination()->Y();
  //   int truck_id = db->get_ready_truck(whid);
  //   if(truck_id<0){
  //     //no truck ready for this warehouse
  //     truck_id = db->get_free_truck();

  //     if(truck_id<0){
  //       //no avaliable truck 
  //       return ;
  //     }
  //     //insert to truck ready map      
  //     std::unordered_map<int,time_t>::const_iterator got = truck_ready.find(truck_id);
  //     if(got==truck_ready.end()){
  //       truck_ready.insert(std::make_pair<int,time_t>(truck_id,get_current_time()));
  //     }
  //   }
  //   else{
  //     //have truck ready for this warehouse

  //   }
  //   //update orders table
  //   if(pr->has_upsaccount()){
  //     int uid = db->get_uid_by_username(pr->upsaccount());
  //     if(uid<0){
  //       //not a valid user
  //       db->add_order(package_id,order_id,whid,des_x,des_y,1,truck_id);
  //     }
  //     db->add_order(package_id,order_id,whid,des_x,des_y,1,truck_id,uid);
  //   }
  //   else{
  //     db->add_order(package_id,order_id,whid,des_x,des_y,1,truck_id);
  //   }
  //   //update items table
  //   for(int i=0;i<pr->items_size();++i){
  //     db->add_item(pr->items(i)->des(),pr->items(i)->count(),order_id);
  //   }
  //   //update truck table
  //   std::string ins("update search_trucks set status=1 where truck_id = ");
  //   ins=ins+std::to_string(truck_id)+";";
  //   db->update(ins);
  // }

  time_t get_current_time(){
    return time(NULL)
  }
}