//
//  SignalTests.cpp
//  Threads
//
//  Created by Gusts Kaksis on 28/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#include "SignalTests.hpp"
#include "Utilities.hpp"
#include "Thread.hpp"
#include "Signal.hpp"

#include <future>

namespace
{
static Logger slog;
}

class Object
{
public:
    Object(const std::string& val)
    {
        data.push_back(val);
    }
    Object(const Object&) = default;
    Object& operator=(const Object&) = default;
    Object(Object&&) = delete;
    Object& operator=(Object&&) = delete;
    ~Object() {}
    
    std::string getVal() const noexcept
    {
        if (data.size())
        {
            return data.at(0);
        }
        return "EMPTY";
    }

private:
    std::vector<std::string> data;
};

void simpleFunction()
{
    slog << "Simple function thread ID: " + tidToStr(std::this_thread::get_id());
}

void argumentFunction(int a , bool b)
{
    slog << "Argument function thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
}

void objectFunction(const Object& obj)
{
    slog << "Object function thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + obj.getVal();
}

class MethodWrapper
{
public:
    void listenSimple()
    {
        slog << "Simple method thread ID: " + tidToStr(std::this_thread::get_id());
    }
    void listenArgs(int a , bool b)
    {
        slog << "Argument method thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
    }
    void listenObject(const Object& obj)
    {
        slog << "Object method thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + obj.getVal();
    }
};

class CustomThread : public gusc::Threads::Thread
{
public:
    void listenSimple()
    {
        slog << "Custom thread - Simple method thread ID: " + tidToStr(std::this_thread::get_id());
    }
    void listenArgs(int a , bool b)
    {
        slog << "Custom thread - Argument method thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
    }
    void listenObject(const Object& obj)
    {
        slog << "Custom thread - Object method thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + obj.getVal();
    }
};

void runSignalTests()
{
    slog << "Signal Tests";
    
    gusc::Threads::ThisThread mt;
    gusc::Threads::Thread t1;
    
    MethodWrapper mw;
    CustomThread ct;
    
    gusc::Threads::Signal<void> sigSimple;
    gusc::Threads::Signal<int, bool> sigArgs;
    gusc::Threads::Signal<Object> sigObject;
    
    // Connect signals on the main thread
    sigSimple.connect(&mt, &simpleFunction);
    sigSimple.connect(&mt, std::bind(&MethodWrapper::listenSimple, &mw));
    sigSimple.connect(&mt, [](){
        slog << "Simple lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    sigArgs.connect(&mt, &argumentFunction);
    sigArgs.connect(&mt, std::bind(&MethodWrapper::listenArgs, &mw, std::placeholders::_1, std::placeholders::_2));
    sigArgs.connect(&mt, [](int a, bool b){
        slog << "Argument lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
    });
    sigObject.connect(&mt, &objectFunction);
    sigObject.connect(&mt, std::bind(&MethodWrapper::listenObject, &mw, std::placeholders::_1));
    sigObject.connect(&mt, [](const Object& o){
        slog << "Object lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + o.getVal();
    });
    // Connect signals on other threads
    sigSimple.connect(&t1, &simpleFunction);
    sigSimple.connect(&t1, std::bind(&MethodWrapper::listenSimple, &mw));
    sigSimple.connect(&t1, [](){
        slog << "Simple lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    sigArgs.connect(&t1, &argumentFunction);
    sigArgs.connect(&t1, std::bind(&MethodWrapper::listenArgs, &mw, std::placeholders::_1, std::placeholders::_2));
    sigArgs.connect(&t1, [](int a, bool b){
        slog << "Argument lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
    });
    sigObject.connect(&t1, &objectFunction);
    sigObject.connect(&t1, std::bind(&MethodWrapper::listenObject, &mw, std::placeholders::_1));
    sigObject.connect(&t1, [](const Object& o){
        slog << "Object lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + o.getVal();
    });
    // Custom threads can be connected directly without binding
    sigSimple.connect(&ct, &CustomThread::listenSimple);
    sigArgs.connect(&ct, &CustomThread::listenArgs);
    sigObject.connect(&ct, &CustomThread::listenObject);
    
    // Emit signal from different threads
    Object o("ASDF");
    sigSimple.emit();
    sigArgs.emit(1, false);
    sigObject.emit(o);
    std::async([&sigSimple, &sigArgs, &sigObject]()
    {
        Object o{"QWERTY"};
        sigSimple.emit();
        sigArgs.emit(2, true);
        sigObject.emit(o);
    });
    
    t1.start();
    mt.stop();
    mt.start();
    
    // Disconnect
    sigSimple.disconnect(&mt, &simpleFunction);
    sigSimple.disconnect(&mt, std::bind(&MethodWrapper::listenSimple, &mw));
    sigArgs.disconnect(&mt, &argumentFunction);
    sigArgs.disconnect(&mt, std::bind(&MethodWrapper::listenArgs, &mw, std::placeholders::_1, std::placeholders::_2));
    sigObject.disconnect(&mt, &objectFunction);
    sigObject.disconnect(&mt, std::bind(&MethodWrapper::listenObject, &mw, std::placeholders::_1));
    sigSimple.disconnect(&t1, &simpleFunction);
    sigSimple.disconnect(&t1, std::bind(&MethodWrapper::listenSimple, &mw));
    sigArgs.disconnect(&t1, &argumentFunction);
    sigArgs.disconnect(&t1, std::bind(&MethodWrapper::listenArgs, &mw, std::placeholders::_1, std::placeholders::_2));
    sigObject.disconnect(&t1, &objectFunction);
    sigObject.disconnect(&t1, std::bind(&MethodWrapper::listenObject, &mw, std::placeholders::_1));
    sigSimple.disconnect(&ct, &CustomThread::listenSimple);
    sigArgs.disconnect(&ct, &CustomThread::listenArgs);
    sigObject.disconnect(&ct, &CustomThread::listenObject);
}
