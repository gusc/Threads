//
//  ThreadExamples.cpp
//  Threads
//
//  Created by Gusts Kaksis on 11/10/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

#if defined(_WIN32)
#   include <Windows.h>
#endif

#include "ThreadExamples.hpp"
#include "Threads/Thread.hpp"

#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

namespace
{

/// This example showcases simple thread procedures with lambdas, how to start threads and how threads will be joined on destruction
void threadProcedureExample()
{
    const auto lambda = [](){
        const auto id = std::this_thread::get_id();
        std::cout << "This is a worker on thread ID: " << id << std::endl;
    };
    gusc::Threads::Thread th1(lambda);
    gusc::Threads::Thread th2(lambda);
    
    const auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << std::endl;
    
    // You have to start threads
    th1.start();
    th2.start();
    
    // Both threads will be joined up on thread object destruction
}

/// This example showcases how to implement your own run-loop using the `Thread::StopToken`
void runLoopExample()
{
    const auto runLoop = [](const gusc::Threads::Thread::StopToken& stopToken){
        while (!stopToken.getIsStopping())
        {
            const auto id = std::this_thread::get_id();
            std::cout << "This is a worker on thread ID: " << id << std::endl;
            std::this_thread::sleep_for(1s);
            //std::this_thread::yield();
        }
    };
    gusc::Threads::Thread th(runLoop);
    
    const auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << std::endl;
    
    // You have to start threads
    th.start();
    
    std::this_thread::sleep_for(10s);
    
    // Signal thread to stop
    th.stop();
    
    // Thread will be joined up on thread object destruction
}

/// This example showcases how to hijack current thread and create a run-loop on it that can be stopped from another thread
void mainThreadExample()
{
    const auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << std::endl;
    
    // Construct an empty ThisThread object
    gusc::Threads::ThisThread mt;
    
    // ThisThread has exposed API to replace thread procedure, so we can capture ourselves
    mt.setThreadProcedure([](const gusc::Threads::Thread::StopToken& stopToken){
        while (!stopToken.getIsStopping())
        {
            const auto id = std::this_thread::get_id();
            std::cout << "This is a worker on thread ID: " << id << std::endl;
            std::this_thread::sleep_for(1s);
            //std::this_thread::yield();
        }
    });
    
    // Create another thread that will signal c
    gusc::Threads::Thread orchestrator([&mt](){
        std::this_thread::sleep_for(10s);
        const auto id = std::this_thread::get_id();
        std::cout << "thread ID: " << id << " notify main thread loop to stop" << std::endl;
        // Stop main run loop
        mt.stop();
    });
    orchestrator.start();
    
    // Start the thread procedure on current thread
    mt.start();
}

}

void runThreadExamples()
{
    threadProcedureExample();
    runLoopExample();
    mainThreadExample();
}
