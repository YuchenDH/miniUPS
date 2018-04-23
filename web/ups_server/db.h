#include <iostream>
#include <fstream>
#include <string>
#include <ctype.h>
#include <pqxx/pqxx>
#include <cstdlib>
#include <ctime>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

using namespace pqxx;

class db : public boost::enable_shared_from_this<db> {
	//add function:0 success, 1, violate unique rule, -1 add fail
 private:
  std::string dbname;
  std::string user;
  std::string password;
  connection *C;
  std::string combine_instruction(){
    std::string temp("dbname=");
    temp+=dbname;
    temp+=" user=";
    temp+=user;
    temp+=" password=";
    temp+=password;
    return temp;
  }
  std::string get_timestamp(){
    time_t nowtime;  
    nowtime = time(NULL); 
    char tmp[64];   
    strftime(tmp,sizeof(tmp),"%F %X",localtime(&nowtime));
    std::string res(tmp);
    res+="+00";   
    return res;
  }
	
 db(std::string Dbname, std::string User, std::string Password) : dbname(Dbname), user(User), password(Password){}
	
 public:
  typedef boost::shared_ptr<db> dbPointer;
  static dbPointer create(std::string Dbname, std::string User, std::string Password) {
    return dbPointer(new db(Dbname, User, Password));
  }
  
  ~db(){
    if (C)
      delete C;
  }
  int connect_db(){
    try{
      C = new connection(combine_instruction());
      if (C->is_open()) {
	std::cout << "Opened database successfully: " << C->dbname() << std::endl;
      } else {
	std::cout << "Can't open database" << std::endl;
	return 1;
      }
    }
    catch(const std::exception &e){
      std::cerr<<e.what()<<std::endl;
      return 1;
    }
    return 0;
  }
  void disconnect_db(){
    C->disconnect();
    std::cout<<"db disconnected\r\n";
  }
  std::string b2s(bool b){
    return b? "true":"false";
  }
  bool verify_truck_id(int id){
    std::string res("select * from search_trucks where truck_id = ");
    res+=std::to_string(id);
    res+=";";
    nontransaction N(*C);
    result R( N.exec( res ));
    return R.size()>0 ? false:true;
  }
  bool verify_tracking_num(long tn){
    std::string res("select * from search_orders where tracking_num = ");
    res+=std::to_string(tn);
    res+=";";
    nontransaction N(*C);
    result R( N.exec( res ));
    return R.size()>0 ? false:true;		
  }
  int add_truck(bool status, int x, int y ,int id){
    if(!verify_truck_id(id)){
      std::cout<<"truck already exists\r\n";
      return 1;
    }
    std::string res("insert into search_trucks (status,xloction,yloction,truck_id) values(");
    try{
      work W(*C);
      res+=b2s(status);res+=",";
      res+=std::to_string(x);res+=",";
      res+=std::to_string(y);res+=",";
      res+=std::to_string(id);res+=");";
      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add truck failed\r\n";
      return -1;
    }	
    return 0;
  }
  int add_truck(bool status,int id){
    if(!verify_truck_id(id)){
      std::cout<<"truck already exists\r\n";
      return 1;
    }
    std::string res("insert into search_trucks (status,truck_id) values(");
    try{
      work W(*C);
      res+=b2s(status);res+=",";
      res+=std::to_string(id);res+=");";
      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add truck failed\r\n";
      return -1;
    }	
    return 0;
  }
  int add_order(long tracking_num,long order_id,int wh_id,int des_x,int des_y,int status,int truck_id,int user_id,std::string first_item,int item_num){
    if(!verify_tracking_num(tracking_num)){
      std::cout<<"order already exists\r\n";
      return 1;
    }
    std::string res("insert into search_orders (tracking_num,order_id,wh_id,des_x,des_y,status,date,first_item,item_num,truck_id,user_id) values(");
    try{
      work W(*C);
      res+=std::to_string(tracking_num);res+=",";
      res+=std::to_string(order_id);res+=",";
      res+=std::to_string(wh_id);res+=",";
      res+=std::to_string(des_x);res+=",";
      res+=std::to_string(des_y);res+=",";
      res+=std::to_string(status);res+=",";
      res+=W.quote(get_timestamp());res+=",";
      res+=W.quote(first_item);res+=",";
      res+=std::to_string(item_num);res+=",";				
      res+=std::to_string(truck_id);res+=",";		
      res+=std::to_string(user_id);res+=");";

      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add order failed\r\n";
      return -1;
    }	
    return 0;		
  }
  int add_order(long tracking_num,long order_id,int wh_id,int status,int truck_id,int user_id,std::string first_item,int item_num){
    if(!verify_tracking_num(tracking_num)){
      std::cout<<"order already exists\r\n";
      return 1;
    }
    std::string res("insert into search_orders (tracking_num,order_id,wh_id,status,date,first_item,item_num,truck_id,user_id) values(");
    try{
      work W(*C);
      res+=std::to_string(tracking_num);res+=",";
      res+=std::to_string(order_id);res+=",";
      res+=std::to_string(wh_id);res+=",";
      res+=std::to_string(status);res+=",";
      res+=W.quote(get_timestamp());res+=",";
      res+=W.quote(first_item);res+=",";
      res+=std::to_string(item_num);res+=",";			
      res+=std::to_string(truck_id);res+=",";
      res+=std::to_string(user_id);res+=");";

      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add order failed\r\n";
      return -1;
    }	
    return 0;		
  }
  int add_order(long tracking_num,long order_id,int wh_id,int des_x,int des_y,int status,int truck_id){
    if(!verify_tracking_num(tracking_num)){
      std::cout<<"order already exists\r\n";
      return 1;
    }
    std::string res("insert into search_orders (tracking_num,order_id,wh_id,des_x,des_y,status,date,truck_id) values(");
    try{
      work W(*C);
      res+=std::to_string(tracking_num);res+=",";
      res+=std::to_string(order_id);res+=",";
      res+=std::to_string(wh_id);res+=",";
      res+=std::to_string(des_x);res+=",";
      res+=std::to_string(des_y);res+=",";
      res+=std::to_string(status);res+=",";
      res+=W.quote(get_timestamp());res+=",";
      res+=std::to_string(truck_id);res+=");";
      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add order failed\r\n";
      return -1;
    }	
    return 0;		
  }
  int add_order(long tracking_num,long order_id,int wh_id,int status,int truck_id){
    if(!verify_tracking_num(tracking_num)){
      std::cout<<"order already exists\r\n";
      return 1;
    }
    std::string res("insert into search_orders (tracking_num,order_id,wh_id,status,date,truck_id) values(");
    try{
      work W(*C);
      res+=std::to_string(tracking_num);res+=",";
      res+=std::to_string(order_id);res+=",";
      res+=std::to_string(wh_id);res+=",";
      res+=std::to_string(status);res+=",";
      res+=W.quote(get_timestamp());res+=",";
      res+=std::to_string(truck_id);res+=");";
      std::cout<<res<<std::endl;
      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add order failed\r\n";
      return -1;
    }	
    return 0;		
  }
  int add_item(std::string name,long order_id){
    std::string res("insert into search_items (name,order_id) values(");
    try{
      work W(*C);
      res+=W.quote(name);res+=",";
      res+=std::to_string(order_id);res+=");";
      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add order failed\r\n";
      return -1;
    }	
    return 0;
  }
  int update(std::string instruction){
    try{
      work W(*C);
      W.exec(instruction);
      W.commit();
    }
    catch(...){
      std::cout<<instruction<<std::endl;
      std::cout<<"update failed\r\n";
      return -1;			
    }
    return 0;
  }
  std::vector<long> * get_oid_by_truckid(int truck_id){
    std::vector<long> * res = new std::vector<long>();
    string ins("SELECT order_id FROM search_orders WHERE truck_id = ");
    ins+=std::to_string(truck_id);ins+=";";
    nontransaction N(*C);
    result R( N.exec( ins ));
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      res->push_back(c[0].as<long>());
    } 
    return res;
  }
};
