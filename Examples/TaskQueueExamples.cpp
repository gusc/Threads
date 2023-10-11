//
//  TaskQueueExamples.cpp
//  Threads
//
//  Created by Gusts Kaksis on 11/10/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

#if defined(_WIN32)
#   include <Windows.h>
#endif

#include "TaskQueueExamples.hpp"
#include "Threads/TaskQueue.hpp"

#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

namespace
{

/// This example showcases simple thread procedures with lambdas, how to start threads and how threads will be joined on destruction
void serialTaskQueueExample()
{
    gusc::Threads::SerialTaskQueue tq;
    
    auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << std::endl;
    
    // Place messages on both threads
    tq.send([](){
        auto id = std::this_thread::get_id();
        std::cout << "This is a task on thread ID: " << id << std::endl;
    });
    
    tq.send([](){
        auto id = std::this_thread::get_id();
        std::cout << "This is a task on thread ID: " << id << std::endl;
    });
    
    // Place a synchronous message - this blocks current thread until the th1 finishes this miessage
    std::cout << "Send wait" << std::endl;
    tq.sendWait([](){
        std::this_thread::sleep_for(5s);
        auto id = std::this_thread::get_id();
        std::cout << "This is a task on thread ID: " << id << std::endl;
    });
    std::cout << "Done" << std::endl;
    
    // Place a message returning a synchronous result
    std::cout << "Send wait" << std::endl;
    auto result1 = tq.sendSync<int>([]() -> int {
        std::this_thread::sleep_for(5s);
        auto id = std::this_thread::get_id();
        std::cout << "This is a task on thread ID: " << id << std::endl;
        return 1;
    });
    std::cout << "Done" << std::endl;
    
    // Place a message returning an asynchronous result via std::future
    std::cout << "Send wait" << std::endl;
    auto f = tq.sendAsync<bool>([]() -> bool {
        std::this_thread::sleep_for(5s);
        auto id = std::this_thread::get_id();
        std::cout << "This is a task on thread ID: " << id << std::endl;
        return true;
    });
    auto result2 = f.getValue();
    std::cout << "Done" << std::endl;
}

}

void runTaskQueueExamples()
{
    serialTaskQueueExample();
}
