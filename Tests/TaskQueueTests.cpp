//
//  TaskQueueTests.cpp
//  Threads
//
//  Created by Gusts Kaksis on 11/11/2022.
//  Copyright © 2022 Gusts Kaksis. All rights reserved.
//

#if defined(_WIN32)
#   include <Windows.h>
#endif

#include "TaskQueueTests.hpp"
#include "Utilities.hpp"
#include "Threads/TaskQueue.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace
{
static Logger tlog;
}

static void callableFunction()
{
    tlog << "Callable function thread ID: " + tidToStr(std::this_thread::get_id());
}

class MethodWrapper
{
public:
    void callableMethod()
    {
        tlog << "Callable method thread ID: " + tidToStr(std::this_thread::get_id());
    }
};

struct CallableStruct
{
    void operator()() const
    {
        tlog << "Callable struct thread ID: " + tidToStr(std::this_thread::get_id());
    }
};

static const auto globalConstLambda = [](){
    tlog << "Global const lambda thread ID: " + tidToStr(std::this_thread::get_id());
};

static auto globalLambda = [](){
    tlog << "Global lambda thread ID: " + tidToStr(std::this_thread::get_id());
};


void runTaskQueueTests()
{
    tlog << "Thread Tests";
    
    gusc::Threads::ThisThread tt;
    
    gusc::Threads::SerialTaskQueue t1;
    gusc::Threads::SerialTaskQueue t2{tt};
    
    tlog << "Main thread ID: " + tidToStr(std::this_thread::get_id());

    // Test function
    callableFunction();
    t1.send(&callableFunction);
    t2.send(&callableFunction);
    
    // Test object method
    MethodWrapper o;
    o.callableMethod();
    t1.send(std::bind(&MethodWrapper::callableMethod, &o));
    t2.send(std::bind(&MethodWrapper::callableMethod, &o));
    
    // Test callable struct
    const CallableStruct cb;
    cb();
    t1.send(cb);
    t2.send(cb);

    // Test callable struct temporary
    t1.send(CallableStruct{});
    t2.send(CallableStruct{});
    
    // Test local lambda
    auto localLambda = [](){
        tlog << "Local lambda thread ID: " + tidToStr(std::this_thread::get_id());
    };
    localLambda();
    t1.send(localLambda);
    t2.send(localLambda);
    
    // Test global lambda
    globalLambda();
    t1.send(globalLambda);
    t2.send(globalLambda);
    
    // Test global const lambda
    globalConstLambda();
    t1.send(globalConstLambda);
    t2.send(globalConstLambda);
    
    // Test anonymous lambda
    t1.sendDelayed([](){
        tlog << "Delayed message on thread ID: " + tidToStr(std::this_thread::get_id());
        tlog.flush();
    }, 1s);
    t2.sendDelayed([](){
        tlog << "Delayed message on thread ID: " + tidToStr(std::this_thread::get_id());
        tlog.flush();
    }, 2s);
    t1.send([](){
        tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t2.send([](){
        tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    auto movable_data1 = std::make_unique<std::vector<int>>(10, 0);
    t1.send([m=std::move(movable_data1)](){
        tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id()) + " X: " + std::to_string(m->size());
    });
    auto movable_data2 = std::make_unique<std::vector<int>>(10, 0);
    t2.send([m=std::move(movable_data2)](){
        tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id()) + " X: " + std::to_string(m->size());
    });
    
    // Test blocking before start
    try
    {
        t1.sendWait([](){
            tlog << "Blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
        });
    }
    catch (const std::exception& ex)
    {
        tlog << ex.what();
    }
    
    // Test blocking calls
    t1.sendWait([](){
        tlog << "Blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t2.sendWait([](){
        tlog << "Blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    auto f1 = t1.sendAsync<int>([&]() -> int {
        // Try to do a blocking call from the same thread
        t1.sendWait([](){
            tlog << "Sub blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
        });
        return 10;
    });
    auto f2 = t2.sendAsync<int>([]() -> int {
        return 20;
    });
    auto res1 = t1.sendSync<int>([&]() -> int {
        // Try to do a blocking call from the same thread
        t1.sendWait([](){
            tlog << "Sub blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
        });
        return 1;
    });
    auto res2 = t2.sendSync<int>([]() -> int {
        return 2;
    });
    tlog << "Sync and async results" << std::to_string(f1.getValue()) << std::to_string(f2.getValue()) << std::to_string(res1) << std::to_string(res2);
    tlog.flush();
    
    t2.sendDelayed([&](){
        tlog << "Stopping serial queue running on ThisThread";
        tt.stop();
    }, 3s);
    tt.start();
    std::this_thread::sleep_for(5s);
    
    // Task queue objects are destrouyed and their threads are stopped
    tlog << "Finishing";
    tlog.flush();
}
