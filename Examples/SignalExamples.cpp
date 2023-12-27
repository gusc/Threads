//
//  SignalExamples.cpp
//  Threads
//
//  Created by Gusts Kaksis on 27/12/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

#if defined(_WIN32)
#   include <Windows.h>
#endif

#include "ThreadExamples.hpp"
#include "Threads/Thread.hpp"
#include "Threads/TaskQueue.hpp"
#include "Threads/Signal.hpp"

#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

namespace
{

class MyObject final
{
public:
    MyObject()
    {}
    
    void someMethod()
    {
        sigSimple.emit();
        sigArgs.emit(1, 2);
    }
    
    // Signal definition includes signal argument types
    gusc::Threads::Signal<void> sigSimple;
    gusc::Threads::Signal<int, int> sigArgs;
};

// TODO: we need to think about this usecase, because shared_from_this is not available in constructor
//class MyObject2 final : public std::enable_shared_from_this<MyObject2>
//                      , public gusc::Threads::SerialTaskQueue
//
//{
//public:
//    MyObject2(MyObject& initOther)
//        : gusc::Threads::SerialTaskQueue("MyObject2::Queue")
//    {
//        initOther.sigArgs.connect(shared_from_this(), std::bind(&MyObject2::onArgs, this, std::placeholders::_1, std::placeholders::_2));
//    }
//
//private:
//    void onArgs(int a, int b)
//    {
//        auto id = std::this_thread::get_id();
//        std::cout << "MyObject2::onArgs thread ID: " << id << "\n";
//    }
//};

}

void runSignalExamples()
{
    auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << "\n";

    gusc::Threads::ThisThread mainThread;
    auto mainThreadQueue = std::make_shared<gusc::Threads::SerialTaskQueue>(mainThread);
    // Queue running on it's own thread
    auto workerQueue = std::make_shared<gusc::Threads::SerialTaskQueue>("OtherQueue");

    MyObject my;
    const auto connection1 = my.sigSimple.connect(workerQueue, [](){
        std::cout << "lambda thread ID: " << std::this_thread::get_id() << "\n";
    });
    const auto connection2 = my.sigSimple.connect(mainThreadQueue, [](){
        std::cout << "lambda thread ID: " << std::this_thread::get_id() << "\n";
    });

    //auto myThread = std::make_shared<MyObject2>(my);

    my.someMethod(); // Emits the signals

    // We need to start our main thread loop so that their signal listeners are executed and then we
    // also need to stop the thread loop to continue execution
    mainThreadQueue->sendDelayed([&](){
        mainThread.stop();
    }, 1s);
    mainThread.start(); // Blocks indefinitely

    connection1->close();
    connection2->close();
}
