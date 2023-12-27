//
//  TaskQueueMocks.hpp
//  Threads
//
//  Created by Gusts Kaksis on 26/12/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

#ifndef TASK_QUEUE_MOCKS_HPP
#   define TASK_QUEUE_MOCKS_HPP

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class TaskQueueMock
{
public:
    MOCK_METHOD(void, call, ());
    MOCK_METHOD(void, noCall, ());
    MOCK_METHOD(void, recover, ());
};

namespace {

class TaskQueueMockWrapper
{
public:
    void setMock(TaskQueueMock* newMock)
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
    TaskQueueMock* actualMock;
};

TaskQueueMockWrapper mock;

std::shared_ptr<std::vector<int>> anon_copyable = std::make_shared<std::vector<int>>(50, 0);
std::unique_ptr<std::vector<int>> anon_movable = std::make_unique<std::vector<int>>(100, 0);

}

void fn()
{
    mock.call();
}

auto lambda = []()
{
    mock.call();
};

const auto lambdaConst = []()
{
    mock.call();
};

const auto lambdaCopyCapture = [c=anon_copyable]()
{
    mock.call();
};

const auto lambdaMoveCapture = [m=std::move(anon_movable)]()
{
    mock.call();
};

struct functor
{
    void operator()() const
    {
        mock.call();
    }
};

class cls
{
public:
    void fn()
    {
        mock.call();
    }
    
    void fnConst() const
    {
        mock.call();
    }
    
    static void fnStatic()
    {
        mock.call();
    }
};

#endif /* TASK_QUEUE_MOCKS_HPP */
