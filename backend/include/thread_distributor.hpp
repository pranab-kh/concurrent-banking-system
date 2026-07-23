#ifndef THREAD_DISTRIBUTOR_HPP
#define THREAD_DISTRIBUTOR_HPP

#include<iostream>
#include <thread>


class Thread_Distributor
{

    unsigned int total_threads;
    unsigned int drogon_distributor;
    unsigned int authorization_workers;
    unsigned int transaction_workers;

    public:
    Thread_Distributor()
    {
        total_threads = std::thread::hardware_concurrency();
        drogon_distributor = total_threads/3;
        authorization_workers = total_threads/3;
        transaction_workers = total_threads/3;

        if(drogon_distributor == 0) drogon_distributor=1;
        if(authorization_workers == 0) authorization_workers=1;
        if(transaction_workers == 0) transaction_workers=1;

    }

    void display()
        {
            std::cout<<"TOTAL THREADS: "<<total_threads<<std::endl;
            std::cout<<"DROGON THREADS: "<<drogon_distributor<<std::endl;
            std::cout<<"AUTHORIZATION THREADS: "<<authorization_workers<<std::endl;
            std::cout<<"TRANSACTION THREADS: "<<transaction_workers<<std::endl;
        }

};

#endif