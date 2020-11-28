//
//  ThreadTests.cpp
//  ThreadSafeSignals
//
//  Created by Gusts Kaksis on 28/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#include "ThreadTests.hpp"
#include "Utilities.hpp"
#include "Thread.hpp"

namespace
{
static Logger log;
}

class CustomThread : public gusc::Threads::Thread
{
};

void callableFunction()
{
    log << "Callable function thread ID: " + tidToStr(std::this_thread::get_id());
}

class MethodWrapper
{
public:
    void callableMethod()
    {
        log << "Callable method thread ID: " + tidToStr(std::this_thread::get_id());
    }
};

struct CallableStruct
{
    void operator()() const
    {
        log << "Callable struct thread ID: " + tidToStr(std::this_thread::get_id());
    }
};

void runThreadTests()
{
    log << "Thread Tests";
        
    gusc::Threads::MainThread mt;
    gusc::Threads::Thread t1;
    CustomThread t2;
    
    log << "Main thread ID: " + tidToStr(std::this_thread::get_id());

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
    
    // Test lambda
    auto lambda = [](){
        log << "Callable lambda thread ID: " + tidToStr(std::this_thread::get_id());
    };
    lambda();
    // can't instantiate the internal templates so we don't do lambdas yet
    // mt.send(lambda);
    mt.send([](){
        log << "Callable lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t1.send([](){
        log << "Callable lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t2.send([](){
        log << "Callable lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    
    // Signal main thread to quit
    mt.quit();
    // Run main runloop
    mt.run();
}