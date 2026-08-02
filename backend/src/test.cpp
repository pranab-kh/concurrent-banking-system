#include<drogon/drogon.h>
#include<iostream>
#include<drogon/WebSocketConnection.h>
#include <drogon/WebSocketController.h>
#include "password_hash.hpp"
#include "database_loader.hpp"





int main()
{
   
    

AccountCreationRequest create;
create.user_id=std::nullopt;
create.full_name = "Spider Man";
create.address = "New York";
create.mobile = "123456789";
create.email = "spiderman@gmail.com";
create.gender = "Male";
create.nid = "143848395";
create.account_type = "SAVING";
create.password = "spiderman";
create.connection = nullptr;


AccountCreationRequest create1;
create1.user_id=4;
create1.full_name = "Spider Man";
create1.address = "New York";
create1.mobile = "123456789";
create1.email = "spiderman@gmail.com";
create1.gender = "Male";
create1.nid = "143848395";
create1.account_type = "SAVING";
create1.password = "spiderman";
create1.connection = nullptr;


Load_DB load;

load.create_account(create1);







    // Thread_Distributor thread_divider;
    // thread_divider.display();

 
    // std::cout<<"Initializing the Universal Port Gateway..."<<std::endl;

    // //configuring the port
    // int port;
    // const char* port_env= std::getenv("PORT");

    // if(port_env != nullptr)
    // {
    //     port = std::stoi(port_env);
    // }
    // else
    // {
    //     port = 8080;
    // }
    // drogon::app().addListener("0.0.0.0",port);

    // // no of thread to handle requests and divide
    // drogon::app().setThreadNum(2);

    // //start of the application loop
    // drogon::app().run();



    return 0;
}