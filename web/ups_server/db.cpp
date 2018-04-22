#include "db.h"
int main(int argc, char ** argv){
  db::dbPointer test = db::create("upsweb","postgres","950703");
  if(test->connect_db()){
    return 1;
  }
  
  test->add_truck(false,100,100,1);
  test->add_truck(true,2);
  test->add_truck(true,1);
  test->add_order(1,1,1,100,100,1,1,1);
  test->add_order(1,2,2,200,200,2,2);
  test->add_order(2,2,2,2,2);
  test->add_item("thing",1);
  
  test->disconnect_db();
  return 0;
}
