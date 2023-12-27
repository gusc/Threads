//
//  ThreadMocks.hpp
//  Threads
//
//  Created by Gusts Kaksis on 26/12/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

#ifndef THEAD_MOCKS_HPP
#   define THEAD_MOCKS_HPP

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class ThreadMock
{
public:
    MOCK_METHOD(void, call, ());
};

namespace {

class ThreadMockWrapper
{
public:
    void setMock(ThreadMock* newMock)
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
private:
    std::mutex mutex;
    ThreadMock* actualMock;
};

ThreadMockWrapper mock;

std::shared_ptr<std::vector<int>> anon_copyable = std::make_shared<std::vector<int>>(50, 0);
std::unique_ptr<std::vector<int>> anon_movable = std::make_unique<std::vector<int>>(100, 0);

}

void fnWithToken(const gusc::Threads::Thread::StopToken&)
{
    mock.call();
}

void fnVoid()
{
    mock.call();
}

void fnArgs(const gusc::Threads::Thread::StopToken&, const std::thread::id& mainThreadId)
{
    mock.call();
    EXPECT_NE(mainThreadId, std::this_thread::get_id());
}

void fnMoveArgs(const gusc::Threads::Thread::StopToken&, std::unique_ptr<std::vector<int>> m)
{
    mock.call();
};

void fnRefArgs(const gusc::Threads::Thread::StopToken&, std::unique_ptr<std::vector<int>>& m)
{
    mock.call();
};

void fnConstRefArgs(const gusc::Threads::Thread::StopToken&, const std::unique_ptr<std::vector<int>>& m)
{
    mock.call();
};

auto lambdaWithToken = [](const gusc::Threads::Thread::StopToken&)
{
    mock.call();
};

const auto lambadConstWithToken = [](const gusc::Threads::Thread::StopToken&)
{
    mock.call();
};

const auto lambdaCopyCapture = [c=anon_copyable](const gusc::Threads::Thread::StopToken&)
{
    mock.call();
};

const auto lambdaMoveCapture = [m=std::move(anon_movable)](const gusc::Threads::Thread::StopToken&)
{
    mock.call();
};

auto lambdaVoid = []()
{
    mock.call();
};

const auto lambdaConstVoid = []()
{
    mock.call();
};

auto lambdaArgs = [](const gusc::Threads::Thread::StopToken&, const std::thread::id& mainThreadId)
{
    mock.call();
    EXPECT_NE(mainThreadId, std::this_thread::get_id());
};

const auto lambdaMoveArgs = [](const gusc::Threads::Thread::StopToken&, std::unique_ptr<std::vector<int>> m)
{
    mock.call();
};

const auto lambdaRefArgs = [](const gusc::Threads::Thread::StopToken&, std::unique_ptr<std::vector<int>>& m)
{
    mock.call();
};

const auto lambdaConstRefArgs = [](const gusc::Threads::Thread::StopToken&, const std::unique_ptr<std::vector<int>>& m)
{
    mock.call();
};

struct functorWithToken
{
    void operator()(const gusc::Threads::Thread::StopToken&) const
    {
        mock.call();
    }
};

struct functorVoid
{
    void operator()() const
    {
        mock.call();
    }
};

struct functorArgs
{
    void operator()(const gusc::Threads::Thread::StopToken&, const std::thread::id& mainThreadId) const
    {
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
    }
};

class cls
{
public:
    void fnWithToken(const gusc::Threads::Thread::StopToken&)
    {
        mock.call();
    }
    
    void fnConstWithToken(const gusc::Threads::Thread::StopToken&) const
    {
        mock.call();
    }
    
    static void fnStaticWithToken(const gusc::Threads::Thread::StopToken&)
    {
        mock.call();
    }
    
    void fnVoid()
    {
        mock.call();
    }
    
    void fnConstVoid() const
    {
        mock.call();
    }
    
    static void fnStaticVoid()
    {
        mock.call();
    }
    
    void fnArgs(const gusc::Threads::Thread::StopToken&, const std::thread::id& mainThreadId)
    {
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
    };

    void fnMoveArgs(const gusc::Threads::Thread::StopToken&, std::unique_ptr<std::vector<int>> m)
    {
        mock.call();
    };
    
    void fnRefArgs(const gusc::Threads::Thread::StopToken&, std::unique_ptr<std::vector<int>>& m)
    {
        mock.call();
    };
    
    void fnConstRefArgs(const gusc::Threads::Thread::StopToken&, const std::unique_ptr<std::vector<int>>& m)
    {
        mock.call();
    };
};

#endif /* THEAD_MOCKS_HPP */
