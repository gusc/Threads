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

void argumentFunction(int a, bool b)
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

static const auto simpleConstLambda = [](){
   slog << "Simple const lambda thread ID: " + tidToStr(std::this_thread::get_id());
};
static const auto argsConstLambda = [](int a, bool b){
   slog << "Argument const lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
};
static const auto objectConstLambda = [](const Object& o){
   slog << "Object const lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + o.getVal();
};

static auto simpleLambda = [](){
   slog << "Simple lambda thread ID: " + tidToStr(std::this_thread::get_id());
};
static const auto argsLambda = [](int a, bool b){
   slog << "Argument lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
};
static const auto objectLambda = [](const Object& o){
   slog << "Object lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + o.getVal();
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
    const auto idxMtSimple2 = sigSimple.connect(&mt, std::bind(&MethodWrapper::listenSimple, &mw));
    const auto idxMtSimple3 = sigSimple.connect(&mt, [](){
        slog << "Simple lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    sigSimple.connect(&mt, simpleLambda);
    sigSimple.connect(&mt, simpleConstLambda);
    
    sigArgs.connect(&mt, &argumentFunction);
    const auto idxMtArgs2 = sigArgs.connect(&mt, std::bind(&MethodWrapper::listenArgs, &mw, std::placeholders::_1, std::placeholders::_2));
    const auto idxMtArgs3 = sigArgs.connect(&mt, [](int a, bool b){
        slog << "Argument lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
    });
    sigArgs.connect(&mt, argsLambda);
    sigArgs.connect(&mt, argsConstLambda);
    
    sigObject.connect(&mt, &objectFunction);
    const auto idxMtObject2 = sigObject.connect(&mt, std::bind(&MethodWrapper::listenObject, &mw, std::placeholders::_1));
    const auto idxMtObject3 = sigObject.connect(&mt, [](const Object& o){
        slog << "Object lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + o.getVal();
    });
    sigObject.connect(&mt, objectLambda);
    sigObject.connect(&mt, objectConstLambda);
    
    // Connect signals on other threads
    
    sigSimple.connect(&t1, &simpleFunction);
    const auto idxT1Simple2 = sigSimple.connect(&t1, std::bind(&MethodWrapper::listenSimple, &mw));
    const auto idxT1Simple3 = sigSimple.connect(&t1, [](){
        slog << "Simple lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    sigSimple.connect(&t1, simpleLambda);
    sigSimple.connect(&t1, simpleConstLambda);
        
    sigArgs.connect(&t1, &argumentFunction);
    const auto idxT1Args2 = sigArgs.connect(&t1, std::bind(&MethodWrapper::listenArgs, &mw, std::placeholders::_1, std::placeholders::_2));
    const auto idxT1Args3 = sigArgs.connect(&t1, [](int a, bool b){
        slog << "Argument lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
    });
    sigArgs.connect(&t1, argsLambda);
    sigArgs.connect(&t1, argsConstLambda);
    
    sigObject.connect(&t1, &objectFunction);
    const auto idxT1Object2 = sigObject.connect(&t1, std::bind(&MethodWrapper::listenObject, &mw, std::placeholders::_1));
    const auto idxT1Object3 = sigObject.connect(&t1, [](const Object& o){
        slog << "Object lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + o.getVal();
    });
    sigObject.connect(&t1, objectLambda);
    sigObject.connect(&t1, objectConstLambda);
    
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
    t1.stop();
    t1.join(); // Let this thread finish up
    
    // Disconnect
    sigSimple.disconnect(&mt, &simpleFunction);
    sigSimple.disconnect(idxMtSimple2);
    sigSimple.disconnect(idxMtSimple3);
    sigSimple.disconnect(&mt, simpleLambda);
    sigSimple.disconnect(&mt, simpleConstLambda);
    
    sigArgs.disconnect(&mt, &argumentFunction);
    sigArgs.disconnect(idxMtArgs2);
    sigArgs.disconnect(idxMtArgs3);
    sigArgs.disconnect(&mt, argsLambda);
    sigArgs.disconnect(&mt, argsConstLambda);
    
    sigObject.disconnect(&mt, &objectFunction);
    sigObject.disconnect(idxMtObject2);
    sigObject.disconnect(idxMtObject3);
    sigObject.disconnect(&mt, objectLambda);
    sigObject.disconnect(&mt, objectConstLambda);
        
    sigSimple.disconnect(&t1, &simpleFunction);
    sigSimple.disconnect(idxT1Simple2);
    sigSimple.disconnect(idxT1Simple3);
    sigSimple.disconnect(&t1, simpleLambda);
    sigSimple.disconnect(&t1, simpleConstLambda);
    
    sigArgs.disconnect(&t1, &argumentFunction);
    sigArgs.disconnect(idxT1Args2);
    sigArgs.disconnect(idxT1Args3);
    sigArgs.disconnect(&t1, argsLambda);
    sigArgs.disconnect(&t1, argsConstLambda);
    
    sigObject.disconnect(&t1, &objectFunction);
    sigObject.disconnect(idxT1Object2);
    sigObject.disconnect(idxT1Object3);
    sigObject.disconnect(&t1, objectLambda);
    sigObject.disconnect(&t1, objectConstLambda);
    
    sigSimple.disconnect(&ct, &CustomThread::listenSimple);
    sigArgs.disconnect(&ct, &CustomThread::listenArgs);
    sigObject.disconnect(&ct, &CustomThread::listenObject);
}
