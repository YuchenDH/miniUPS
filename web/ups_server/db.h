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

struct package{
  int x;
  int y;
  long package_id;
};
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
    //temp+=" password=";
    // temp+=password;
    temp+=" host=db port=5432";
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
	
 db(std::string Dbname, std::string User) : dbname(Dbname), user(User){}
	
 public:
  typedef boost::shared_ptr<db> dbPointer;
  static dbPointer create(std::string Dbname, std::string User) {
    return dbPointer(new db(Dbname, User));
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
  int add_truck(int status, int x, int y ,int id){
    if(!verify_truck_id(id)){
      std::cout<<"truck already exists\r\n";
      return 1;
    }
    std::string res("insert into search_trucks (status,xloction,yloction,truck_id) values(");
    try{
      work W(*C);
      res+=std::to_string(status);res+=",";
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
    std::cout<<res<<" success\r\n";	
    return 0;
  }
  int add_truck(int status,int id){
    if(!verify_truck_id(id)){
      std::cout<<"truck already exists\r\n";
      return 1;
    }
    std::string res("insert into search_trucks (status,truck_id) values(");
    try{
      work W(*C);
      res+=std::to_string(status);res+=",";
      res+=std::to_string(id);res+=");";
      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add truck failed\r\n";
      return -1;
    }	
    std::cout<<res<<" success\r\n";
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
    std::cout<<res<<" success\r\n";	
    return 0;		
  }
  int add_order(long tracking_num,long order_id,int wh_id,int des_x,int des_y,int status,int truck_id,std::string first_item,int item_num){
    if(!verify_tracking_num(tracking_num)){
      std::cout<<"order already exists\r\n";
      return 1;
    }
    std::string res("insert into search_orders (tracking_num,order_id,wh_id,des_x,des_y,status,date,first_item,item_num,truck_id) values(");
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
      res+=std::to_string(truck_id);res+=");";		
      // res+=std::to_string(user_id);res+=");";

      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add order failed\r\n";
      return -1;
    }
    std::cout<<res<<" success\r\n";	
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
    std::cout<<res<<" success\r\n";	
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
    std::cout<<res<<" success\r\n";	
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
    std::cout<<res<<" success\r\n";	
    return 0;		
  }
  int add_item(std::string name,int count,long order_id){
    std::string res("insert into search_items (name,count,order_id) values(");
    try{
      work W(*C);
      res+=W.quote(name);res+=",";
      res+=std::to_string(count);res+=",";
      res+=std::to_string(order_id);res+=");";
      W.exec(res);
      W.commit();	
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add item failed\r\n";
      return -1;
    }
    std::cout<<res<<" success\r\n";	
    return 0;
  }
  int add_warehouse(int id, int x, int y){
    std::string res("insert into search_warehouses (wh_id,xlocation,ylocation) values(");
    try{
      work W(*C);
      res+=std::to_string(id);res+=",";
      res+=std::to_string(x);res+=",";
      res+=std::to_string(y);res+=");";
      W.exec(res);
      W.commit(); 
    }
    catch(...){
      std::cout<<res<<std::endl;
      std::cout<<"add warehouse failed\r\n";
      return -1;
    }
    std::cout<<res<<" success\r\n"; 
    return 0;    
  }
  int update(std::string instruction){
    std::cout<<"update begin\r\n";
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
    std::cout<<instruction<<" success\r\n";
    return 0;
  }
  int get_oldest_order_whid(){
    std::string ins("select wh_id from search_orders where truck_id=-1 order by date;");
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    if(c != R.end()){
      std::cout<<ins<<" success; get "<<c[0].as<int>()<<"\r\n";
      return c[0].as<int>();
    }
    else{
      return -1;
    }       
  }
  std::vector<long> * get_oid_by_whid(int wh_id){
    std::vector<long> * res = new std::vector<long>();
    std::string ins("SELECT order_id FROM search_orders WHERE truck_id = -1 and wh_id = ");
    ins+=std::to_string(wh_id);ins+=";";
    work W(*C);
    result R( W.exec( ins ));W.commit();
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      res->push_back(c[0].as<long>());
    }
    std::cout<<ins<<" success\r\n(";
    for(size_t i=0;i<res->size();++i){
      std::cout<<res->at(i)<<", ";
    }
    std::cout<<")\r\n";
    return res;
  }  
  std::vector<long> * get_oid_by_truckid(int truck_id){
    std::vector<long> * res = new std::vector<long>();
    std::string ins("SELECT order_id FROM search_orders WHERE status = 2 and truck_id = ");
    ins+=std::to_string(truck_id);ins+=";";
    work W(*C);
    result R( W.exec( ins ));W.commit();
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      res->push_back(c[0].as<long>());
    } 
    std::cout<<ins<<" success\r\n(";
    for(size_t i=0;i<res->size();++i){
      std::cout<<res->at(i)<<", ";
    }
    std::cout<<")\r\n";
    return res;
  }
  int get_free_truck(){
    //-1 means not avaliable truck
    std::string ins("SELECT truck_id FROM search_trucks WHERE status = 0");
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    if(c != R.end()){
      std::cout<<ins<<" success; get "<<c[0].as<int>()<<"\r\n";
      return c[0].as<int>();
    }
    else{
      return -1;
    }    
  }
  int get_ready_truck(int whid){
    std::string ins("select search_trucks.truck_id from search_orders,search_trucks where search_trucks.status = 1 and search_orders.truck_id = search_trucks.truck_id and search_orders.wh_id = ");
    ins+=std::to_string(whid);ins+=";";
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    if(c != R.end()){
      std::cout<<ins<<" success; get "<<c[0].as<int>()<<"\r\n";
      return c[0].as<int>();
    }
    else{
      return -1;
    }    
  }
  int get_uid_by_username(std::string username){
    //-1 means no valid user
    std::string ins("SELECT id FROM weblog_realuser WHERE username = \'");
    ins+=username;ins+="\';";
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    if(c != R.end()){
      std::cout<<ins<<" success; get "<<c[0].as<int>()<<"\r\n";
      return c[0].as<int>();
    }
    else{
      return -1;
    } 
  }
  int get_truck_status(int truck_id){
    std::string ins("SELECT status FROM search_trucks WHERE truck_id = ");
    ins+=std::to_string(truck_id);ins+=";";
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    if(c != R.end()){
      std::cout<<ins<<" success; get "<<c[0].as<int>()<<"\r\n";
      return c[0].as<int>();
    }
    else{
      std::cout<<"no truck with id = "<<truck_id<<"\r\n";
      return -1;
    }     
  }
  int get_warehouse_id(int x,int y){
    std::string ins("select wh_id from search_warehouses where xlocation = ");
    ins+=std::to_string(x);
    ins+=" and ylocation = ";
    ins+=std::to_string(y);
    ins+=";";
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    if(c != R.end()){
      std::cout<<ins<<" success; get "<<c[0].as<int>()<<"\r\n";
      return c[0].as<int>();
    }
    else{
      return -1;
    }    
  }
  long get_pid_by_oid(long oid){
    std::string ins("SELECT tracking_num FROM search_orders WHERE order_id = ");
    ins+=std::to_string(oid);ins+=";";
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    if(c != R.end()){
      std::cout<<ins<<" success; get "<<c[0].as<int>()<<"\r\n";
      return c[0].as<long>();
    }
    else{
      return -1;
    }     
  }
  bool has_unprocessed_order(){
    std::string ins("select order_id from search_orders where truck_id=-1;");
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    if(c != R.end()){
      std::cout<<ins<<" success; get true\r\n";
      return true;
    }
    else{
      std::cout<<ins<<" success; get false\r\n";
      return false;
    }
  }
  std::vector<package*> * get_package_by_truck(int truck_id){
    std::vector<package*> * res = new std::vector<package*>();
    std::string ins("select tracking_num,des_x,des_y from search_orders where status = 3 and truck_id =");
    ins+=std::to_string(truck_id);
    ins+=";";
    work W(*C);
    result R( W.exec( ins ));W.commit();
    result::const_iterator c = R.begin();
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      package temp;
      temp.package_id=c[0].as<long>();
      temp.x=c[1].as<int>();
      temp.y=c[2].as<int>();
      res->push_back(&temp);
    } 
    std::cout<<ins<<" success\r\n";    
    return res;
  }
};
