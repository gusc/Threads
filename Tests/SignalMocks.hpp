//
//  SignalMocks.hpp
//  Threads
//
//  Created by Gusts Kaksis on 26/12/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

#ifndef SIGNAL_MOCKS_HPP
#   define SIGNAL_MOCKS_HPP

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class SignalMock
{
public:
    MOCK_METHOD(void, call, ());
    MOCK_METHOD(void, noCall, ());
    MOCK_METHOD(void, recover, ());
};

namespace {

class SignalMockWrapper
{
public:
    void setMock(SignalMock* newMock)
    {
        std::lock_guard lock { mutex };
        actualMock = newMock;
    }

    void call()
    {
        std::lock_guard lock { mutex };
        if (actualMock)
        {
            actualMock->call();
        }
    }

    void noCall()
    {
        std::lock_guard lock { mutex };
        if (actualMock)
        {
            actualMock->noCall();
        }
    }

    void recover()
    {
        std::lock_guard lock { mutex };
        if (actualMock)
        {
            actualMock->recover();
        }
    }
private:
    std::mutex mutex;
    SignalMock* actualMock;
};

SignalMockWrapper mock;

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
    mock.call();
}

void argumentFunction(int a, bool b)
{
    mock.call();
}

void objectFunction(const Object& obj)
{
    mock.call();
}

class MethodWrapper
{
public:
    void listenSimple()
    {
        mock.call();
    }
    void listenArgs(int a , bool b)
    {
        mock.call();
    }
    void listenObject(const Object& obj)
    {
        mock.call();
    }
};

class InheritedQueue : public gusc::Threads::SerialTaskQueue
{
public:
    void listenSimple()
    {
        mock.call();
    }
    void listenArgs(int a , bool b)
    {
        mock.call();
    }
    void listenObject(const Object& obj)
    {
        mock.call();
    }
};

static const auto simpleConstLambda = [](){
    mock.call();
};
static const auto argsConstLambda = [](int a, bool b){
    mock.call();
};
static const auto objectConstLambda = [](const Object& o){
    mock.call();
};

static auto simpleLambda = [](){
    mock.call();
};
static const auto argsLambda = [](int a, bool b){
    mock.call();
};
static const auto objectLambda = [](const Object& o){
    mock.call();
};

#endif /* SIGNAL_MOCKS_HPP */
