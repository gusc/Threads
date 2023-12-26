//
//  SignalTests.cpp
//  Threads
//
//  Created by Gusts Kaksis on 28/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#if defined(_WIN32)
#   include <Windows.h>
#endif

#include <gtest/gtest.h>

#include "Utilities.hpp"
#include "Threads/Thread.hpp"
#include "Threads/Signal.hpp"

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
    Object(Object&&) = default;
    Object& operator=(Object&&) = default;
    ~Object() = default;
    
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

class CustomThread : public gusc::Threads::SerialTaskQueue
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

TEST(ThreadTests, Signal)
{
    slog << "Signal Tests";
    
    gusc::Threads::ThisThread tt;
    
    auto mt = std::make_shared<gusc::Threads::SerialTaskQueue>(tt);
    auto t1 = std::make_shared<gusc::Threads::SerialTaskQueue>();
    
    MethodWrapper mw;
    auto ct = std::make_shared<CustomThread>();
    
    gusc::Threads::Signal<void> sigSimple;
    gusc::Threads::Signal<int, bool> sigArgs;
    gusc::Threads::Signal<Object> sigObject;
    
    // Connect signals on the main thread
    
    auto conMtSimple1 = sigSimple.connect(mt, &simpleFunction);
    auto conMtSimple2 = sigSimple.connect(mt, std::bind(&MethodWrapper::listenSimple, &mw));
    auto conMtSimple3 = sigSimple.connect(mt, [](){
        slog << "Simple lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    auto conMtSimple4 = sigSimple.connect(mt, simpleLambda);
    auto conMtSimple5 = sigSimple.connect(mt, simpleConstLambda);
    
    auto conMtArgs1 = sigArgs.connect(mt, &argumentFunction);
    auto conMtArgs2 = sigArgs.connect(mt, std::bind(&MethodWrapper::listenArgs, &mw, std::placeholders::_1, std::placeholders::_2));
    auto conMtArgs3 = sigArgs.connect(mt, [](int a, bool b){
        slog << "Argument lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
    });
    auto conMtArgs4 = sigArgs.connect(mt, argsLambda);
    auto conMtArgs5 = sigArgs.connect(mt, argsConstLambda);
    
    auto conMtObject1 = sigObject.connect(mt, &objectFunction);
    auto conMtObject2 = sigObject.connect(mt, std::bind(&MethodWrapper::listenObject, &mw, std::placeholders::_1));
    auto conMtObject3 = sigObject.connect(mt, [](const Object& o){
        slog << "Object lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + o.getVal();
    });
    auto conMtObject4 = sigObject.connect(mt, objectLambda);
    auto conMtObject5 = sigObject.connect(mt, objectConstLambda);
    
    // Connect signals on other threads
    
    auto conT1Simple1 = sigSimple.connect(t1, &simpleFunction);
    auto conT1Simple2 = sigSimple.connect(t1, std::bind(&MethodWrapper::listenSimple, &mw));
    auto conT1Simple3 = sigSimple.connect(t1, [](){
        slog << "Simple lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    auto conT1Simple4 = sigSimple.connect(t1, simpleLambda);
    auto conT1Simple5 = sigSimple.connect(t1, simpleConstLambda);
        
    auto conT1Args1 = sigArgs.connect(t1, &argumentFunction);
    auto conT1Args2 = sigArgs.connect(t1, std::bind(&MethodWrapper::listenArgs, &mw, std::placeholders::_1, std::placeholders::_2));
    auto conT1Args3 = sigArgs.connect(t1, [](int a, bool b){
        slog << "Argument lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + std::to_string(a) + ", " + std::to_string(b);
    });
    auto conT1Args4 = sigArgs.connect(t1, argsLambda);
    auto conT1Args5 = sigArgs.connect(t1, argsConstLambda);
    
    auto conT1Object1 = sigObject.connect(t1, &objectFunction);
    auto conT1Object2 = sigObject.connect(t1, std::bind(&MethodWrapper::listenObject, &mw, std::placeholders::_1));
    auto conT1Object3 = sigObject.connect(t1, [](const Object& o){
        slog << "Object lambda thread ID: " + tidToStr(std::this_thread::get_id()) + ", " + o.getVal();
    });
    auto conT1Object4 = sigObject.connect(t1, objectLambda);
    auto conT1Object5 = sigObject.connect(t1, objectConstLambda);
    
    // Custom threads can be connected directly without binding
    
    auto conCtSimple = sigSimple.connect(ct, std::bind(&CustomThread::listenSimple, ct.get()));
    auto conCtArgs = sigArgs.connect(ct, std::bind(&CustomThread::listenArgs, ct.get(), std::placeholders::_1, std::placeholders::_2));
    auto conCtObject = sigObject.connect(ct, std::bind(&CustomThread::listenObject, ct.get(), std::placeholders::_1));
    
    // Emit signal from different threads
    Object o("ASDF");
    sigSimple.emit();
    sigArgs.emit(1, false);
    sigObject.emit(o);
    auto future = std::async([&sigSimple, &sigArgs, &sigObject]()
    {
        Object o{"QWERTY"};
        sigSimple.emit();
        sigArgs.emit(2, true);
        sigObject.emit(o);
    });
    
    // Stop main thread runloop aswell
    mt->send([&](){
        tt.stop();
    });
    tt.start();
    
    // Disconnect
    conMtSimple1->close();
    conMtSimple2->close();
    conMtSimple3->close();
    conMtSimple4->close();
    conMtSimple5->close();
    
    conMtArgs1->close();
    conMtArgs2->close();
    conMtArgs3->close();
    conMtArgs4->close();
    conMtArgs5->close();
    
    conMtObject1->close();
    conMtObject2->close();
    conMtObject3->close();
    conMtObject4->close();
    conMtObject5->close();
    
    conT1Simple1->close();
    conT1Simple2->close();
    conT1Simple3->close();
    conT1Simple4->close();
    conT1Simple5->close();
    
    conT1Args1->close();
    conT1Args2->close();
    conT1Args3->close();
    conT1Args4->close();
    conT1Args5->close();
    
    conT1Object1->close();
    conT1Object2->close();
    conT1Object3->close();
    conT1Object4->close();
    conT1Object5->close();
    
    conCtSimple->close();
    conCtArgs->close();
    conCtObject->close();
}

