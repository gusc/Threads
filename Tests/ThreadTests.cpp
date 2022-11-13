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
#include "Threads/ThreadPool.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace
{
static Logger tlog;
}

class CustomThread : public gusc::Threads::Thread
{
};

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


void runThreadTests()
{
    tlog << "Thread Tests";
        
    gusc::Threads::ThisThread mt;
    gusc::Threads::Thread t1;
    CustomThread t2;
    gusc::Threads::ThreadPool tp(3);
    
    // Start all threads
    t1.start();
    t2.start();
    tp.start();
    
    tlog << "Main thread ID: " + tidToStr(std::this_thread::get_id());

    // Test function
    callableFunction();
    mt.run(&callableFunction);
    t1.run(&callableFunction);
    t2.run(&callableFunction);
    tp.run(&callableFunction);
    tp.run(&callableFunction);
    tp.run(&callableFunction);
    
    
    // Test object method
    MethodWrapper o;
    o.callableMethod();
    mt.run(std::bind(&MethodWrapper::callableMethod, &o));
    t1.run(std::bind(&MethodWrapper::callableMethod, &o));
    t2.run(std::bind(&MethodWrapper::callableMethod, &o));
    tp.run(std::bind(&MethodWrapper::callableMethod, &o));
    tp.run(std::bind(&MethodWrapper::callableMethod, &o));
    tp.run(std::bind(&MethodWrapper::callableMethod, &o));
    
    // Test callable struct
    const CallableStruct cb;
    cb();
    mt.run(cb);
    t1.run(cb);
    t2.run(cb);
    tp.run(cb);
    tp.run(cb);
    tp.run(cb);

    // Test callable struct temporary
    mt.run(CallableStruct{});
    t1.run(CallableStruct{});
    t2.run(CallableStruct{});
    tp.run(CallableStruct{});
    tp.run(CallableStruct{});
    tp.run(CallableStruct{});
    
    // Test local lambda
    auto localLambda = [](){
        tlog << "Local lambda thread ID: " + tidToStr(std::this_thread::get_id());
    };
    localLambda();
    mt.run(localLambda);
    t1.run(localLambda);
    t2.run(localLambda);
    tp.run(localLambda);
    tp.run(localLambda);
    tp.run(localLambda);
    
    // Test global lambda
    globalLambda();
    mt.run(globalLambda);
    t1.run(globalLambda);
    t2.run(globalLambda);
    tp.run(globalLambda);
    tp.run(globalLambda);
    tp.run(globalLambda);
    
    // Test global const lambda
    globalConstLambda();
    mt.run(globalConstLambda);
    t1.run(globalConstLambda);
    t2.run(globalConstLambda);
    tp.run(globalConstLambda);
    tp.run(globalConstLambda);
    tp.run(globalConstLambda);
    
    // Test anonymous lambda
    mt.run([](){
        tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t1.run([](){
        tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t2.run([](){
        tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    tp.run([](){
        tlog << "Anonymous lambda 1 thread ID: " + tidToStr(std::this_thread::get_id());
    });
    tp.run([](){
        tlog << "Anonymous lambda 2 thread ID: " + tidToStr(std::this_thread::get_id());
    });
    tp.run([](){
        tlog << "Anonymous lambda 3 thread ID: " + tidToStr(std::this_thread::get_id());
    });
    
    // Post a stop on all the threads
    mt.run([&](){
        tlog << "Stop thread ID: " + tidToStr(std::this_thread::get_id());
        mt.stop();
    });
    t1.run([&](){
        tlog << "Stop thread ID: " + tidToStr(std::this_thread::get_id());
        t1.stop();
    });
    t2.run([&](){
        tlog << "Stop thread ID: " + tidToStr(std::this_thread::get_id());
        t2.stop();
    });
    tp.run([&](){
        tlog << "Stop thread pool";
        tp.stop();
    });
    
    // Start this-thread loop
    mt.start();
    
    tlog.flush();
    
    std::this_thread::sleep_for(1s);
}
