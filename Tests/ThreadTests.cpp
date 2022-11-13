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

static void callableFunction(const gusc::Threads::Thread::StopToken&)
{
    tlog << "Callable function thread ID: " + tidToStr(std::this_thread::get_id());
}

class MethodWrapper
{
public:
    void callableMethod(const gusc::Threads::Thread::StopToken&)
    {
        tlog << "Callable method thread ID: " + tidToStr(std::this_thread::get_id());
    }
};

struct CallableStruct
{
    void operator()(const gusc::Threads::Thread::StopToken&) const
    {
        tlog << "Callable struct thread ID: " + tidToStr(std::this_thread::get_id());
    }
};

static const auto globalConstLambda = [](const gusc::Threads::Thread::StopToken&){
    tlog << "Global const lambda thread ID: " + tidToStr(std::this_thread::get_id());
};

static auto globalLambda = [](const gusc::Threads::Thread::StopToken&){
    tlog << "Global lambda thread ID: " + tidToStr(std::this_thread::get_id());
};


void runThreadTests()
{
    tlog << "Thread Tests";
    tlog << "Main thread ID: " + tidToStr(std::this_thread::get_id());

    gusc::Threads::Thread::StopToken token;
    
    // Test simple function
    {
        callableFunction(token);
        gusc::Threads::Thread t(callableFunction);
        t.start();
    }
    
    // Test object method
    {
        MethodWrapper o;
        o.callableMethod(token);
        gusc::Threads::Thread t(std::bind(&MethodWrapper::callableMethod, &o, std::placeholders::_1));
        t.start();
    }
    
    // Test callable struct
    {
        const CallableStruct cb;
        cb(token);
        gusc::Threads::Thread t(cb);
        t.start();
    }

    // Test callable struct temporary
    {
        gusc::Threads::Thread t(CallableStruct{});
        t.start();
    }
    
    // Test local lambda
    {
        auto localLambda = [](const gusc::Threads::Thread::StopToken&){
            tlog << "Local lambda thread ID: " + tidToStr(std::this_thread::get_id());
        };
        localLambda(token);
        gusc::Threads::Thread t(localLambda);
        t.start();
    }
    
    // Test global lambda
    {
        globalLambda(token);
        gusc::Threads::Thread t(globalLambda);
        t.start();
    }
    
    // Test global const lambda
    {
        globalConstLambda(token);
        gusc::Threads::Thread t(globalConstLambda);
        t.start();
    }
    
    // Test anonymous lambda
    {
        gusc::Threads::Thread t([](const gusc::Threads::Thread::StopToken&){
            tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
        });
        t.start();
    }
    
    tlog.flush();
}
