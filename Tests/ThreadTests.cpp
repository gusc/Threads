//
//  ThreadTests.cpp
//  Threads
//
//  Created by Gusts Kaksis on 28/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#include "ThreadTests.hpp"
#include "Utilities.hpp"
#include "Threads/Thread.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace
{
static Logger tlog;
}

class CustomThread : public gusc::Threads::Thread
{
};

void callableFunction()
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


void runThreadTests()
{
    tlog << "Thread Tests";
        
    gusc::Threads::ThisThread mt;
    gusc::Threads::Thread t1;
    CustomThread t2;
    
    tlog << "Main thread ID: " + tidToStr(std::this_thread::get_id());

    // Test function
    callableFunction();
    mt.send(&callableFunction);
    t1.send(&callableFunction);
    t2.send(&callableFunction);
    
    // Test object method
    MethodWrapper o;
    o.callableMethod();
    mt.send(std::bind(&MethodWrapper::callableMethod, &o));
    t1.send(std::bind(&MethodWrapper::callableMethod, &o));
    t2.send(std::bind(&MethodWrapper::callableMethod, &o));
    
    // Test callable struct
    const CallableStruct cb;
    cb();
    mt.send(cb);
    t1.send(cb);
    t2.send(cb);

    // Test callable struct temporary
    mt.send(CallableStruct{});
    t1.send(CallableStruct{});
    t2.send(CallableStruct{});
    
    // Test local lambda
    auto localLambda = [](){
        tlog << "Local lambda thread ID: " + tidToStr(std::this_thread::get_id());
    };
    localLambda();
    mt.send(localLambda);
    t1.send(localLambda);
    t2.send(localLambda);
    
    // Test global lambda
    globalLambda();
    mt.send(globalLambda);
    t1.send(globalLambda);
    t2.send(globalLambda);
    
    // Test global const lambda
    globalConstLambda();
    mt.send(globalConstLambda);
    t1.send(globalConstLambda);
    t2.send(globalConstLambda);
    
    // Test anonymous lambda
    mt.send([](){
        tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
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
    
    mt.sendDelayed([&](){
        tlog << "Stopping thread mt";
        tlog.flush();
        mt.stop();
    }, 5s);
    
    // Signal main thread to quit (this effectivelly stops processing all the messages)
    mt.stop();
    // Start all threads and main run-loop
    t1.start();
    t2.start();
    
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
    tlog << "Sync and async results" << std::to_string(f1.get()) << std::to_string(f2.get()) << std::to_string(res1) << std::to_string(res2);
    tlog.flush();
    
    t1.sendDelayed([&](){
        tlog << "Stopping thread t1";
        tlog.flush();
        t1.stop();
    }, 5s);
    t2.sendDelayed([&](){
        tlog << "Stopping thread t2";
        tlog.flush();
        t2.stop();
    }, 5s);
    
    mt.start();
    std::this_thread::sleep_for(3s);
}
