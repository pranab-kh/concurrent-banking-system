// #include<drogon/drogon.h>
#include<iostream>
// #include"web_socket_controllers.hpp"
#include <thread>
#include "thread_distributor.hpp"



int main()
{
    Thread_Distributor thread_divider;
    thread_divider.display();

 
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