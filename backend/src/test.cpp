#include<drogon/drogon.h>
#include<iostream>


int main()
{
    std::cout<<"Initializing the Universal Port Gateway..."<<std::endl;

    //configuring the port
    drogon::app().addListener("0.0.0.0",8080);

    // no of thread to handle requests and divide
    drogon::app().setThreadNum(2);

    //start of the application loop
    drogon::app().run();



    return 0;
}