//
//  main.cpp
//  ThreadSafeSignals
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#include <iostream>
#include <thread>
#include <future>

#include "Signal.hpp"
#include "Thread.hpp"

void foo()
{
    auto id = std::this_thread::get_id();
    std::cout << "Foo thread ID: " << id << std::endl;
}

class MyThread : public gusc::Threads::Thread
{
public:
    MyThread() : Thread()
    {
        sigResult.connect(this, &MyThread::onResult);
    }
    
    gusc::Threads::Signal<int> sigResult;
    
private:
    void onResult(int res)
    {
        auto id = std::this_thread::get_id();
        std::cout << "Res: " << res << " thread ID: " << id << std::endl;
    }
    
};

int main(int argc, const char * argv[]) {
    
    gusc::Threads::MainThread mt;
    gusc::Threads::Thread t1;
    MyThread t2;
    
    auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << std::endl;
    
    gusc::Threads::Signal<void> simpleSignal;
    simpleSignal.connect(&t1, &foo);
    
    gusc::Threads::Signal<int, int> mainSignal;
    mainSignal.connect(&mt, [](int a, int b)
    {
        auto id = std::this_thread::get_id();
        std::cout << "a: " << a << ", b: " << b << " thread ID: " << id << std::endl;
    });   
    
    std::async([&simpleSignal, &t2, &mt, &mainSignal]()
    {
        auto id = std::this_thread::get_id();
        std::cout << "Async thread ID: " << id << std::endl;
        
        simpleSignal.emit();
        t2.sigResult.emit(1);
        mainSignal.emit(1, 3);
        
        mt.quit();
    });

    simpleSignal.emit();
    t2.sigResult.emit(2);
    mainSignal.emit(2, 4);
    
    std::cout << "Ener main runloop" << std::endl;
        
    mt.run();
    return 0;
}
