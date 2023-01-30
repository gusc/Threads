//
//  ThreadTests.cpp
//  Threads
//
//  Created by Gusts Kaksis on 28/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#if defined(_WIN32)
#   include <Windows.h>
#endif

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
        // Can't use std::bind because MSVC is stupid
        gusc::Threads::Thread t([&](const gusc::Threads::Thread::StopToken& token) {
            o.callableMethod(token);
        });
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
    
    // Test ThisThread
    {
        gusc::Threads::ThisThread tt;
        tt.setThreadProcedure([&](const gusc::Threads::Thread::StopToken& token){
            tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
            tt.stop();
            if (token.getIsStopping())
            {
                tlog << "This thread has been informed to stop";
            }
        });
        tt.start();
    }
    
    // Test ThreadPool
    {
        auto tp = gusc::Threads::ThreadPool(2, [&](const gusc::Threads::Thread::StopToken& token){
            tlog << "Running on a thread pool thread ID: " + tidToStr(std::this_thread::get_id());
        });
    }
    
    // Test start/stop
    {
        gusc::Threads::Thread t([](const gusc::Threads::Thread::StopToken&){
            tlog << "Starting thread ID: " + tidToStr(std::this_thread::get_id());
        });
        t.start();
        t.stop();
        t.join();
        t.start();
        t.stop();
        t.join();
    }
    
    tlog.flush();
}
