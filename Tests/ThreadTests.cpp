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

#include <gtest/gtest.h>

#include "Utilities.hpp"
#include "Threads/Thread.hpp"
#include "Threads/ThreadPool.hpp"
#include "ThreadMocks.hpp"

#include <chrono>
#include <atomic>

using namespace std::chrono_literals;

TEST(ThreadTests, Functions)
{
    ThreadMock actualMock;
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(6);

    {
        gusc::Threads::Thread t(&fnWithToken);
        t.start();
    }
    {
        gusc::Threads::Thread t(&fnVoid);
        t.start();
    }
    {
        gusc::Threads::Thread t(&fnArgs, std::this_thread::get_id());
        t.start();
    }
    {
        auto movable_data = std::make_unique<std::vector<int>>(40, 0);
        // Because of the Thread's ability to be started and stopped we can not pass movable objects as thread procedure's arguments
        // as they will be moved away from internal strucutre on the first run, to avoid confusion this use-case has been removed
        //        gusc::Threads::Thread t(&fnMoveArgs, std::move(movable_data));
        //        t.start();
        fnMoveArgs({}, std::move(movable_data));
    }
    {
        auto referable_data = std::make_unique<std::vector<int>>(40, 0);
        gusc::Threads::Thread t(&fnRefArgs, std::ref(referable_data));
        t.start();
    }
    {
        auto referable_data = std::make_unique<std::vector<int>>(40, 0);
        gusc::Threads::Thread t(&fnConstRefArgs, std::ref(referable_data));
        t.start();
    }
    mock.setMock(nullptr);
}

TEST(ThreadTests, Lambdas)
{
    ThreadMock actualMock;
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(13);

    {
        gusc::Threads::Thread t(lambdaWithToken);
        t.start();
    }
    {
        gusc::Threads::Thread t(lambadConstWithToken);
        t.start();
    }
    {
        gusc::Threads::Thread t(lambdaCopyCapture);
        t.start();
    }
    {
        gusc::Threads::Thread t(lambdaMoveCapture);
        t.start();
    }
    {
        gusc::Threads::Thread t(lambdaVoid);
        t.start();
    }
    {
        gusc::Threads::Thread t(lambdaConstVoid);
        t.start();
    }
    {
        gusc::Threads::Thread t(lambdaArgs, std::this_thread::get_id());
        t.start();
    }
    {
        auto movable_data = std::make_unique<std::vector<int>>(40, 0);
        // Because of the Thread's ability to be started and stopped we can not pass movable objects as thread procedure's arguments
        // as they will be moved away from internal strucutre on the first run, to avoid confusion this use-case has been removed
        //        gusc::Threads::Thread t(lambdaMoveArgs, std::move(movable_data));
        //        t.start();
        lambdaMoveArgs({}, std::move(movable_data));
    }
    {
        auto referable_data = std::make_unique<std::vector<int>>(40, 0);
        gusc::Threads::Thread t(lambdaRefArgs, std::ref(referable_data));
        t.start();
    }
    {
        auto referable_data = std::make_unique<std::vector<int>>(40, 0);
        gusc::Threads::Thread t(lambdaConstRefArgs, std::ref(referable_data));
        t.start();
    }
    {
        auto mainThreadId = std::this_thread::get_id();
        auto local = [mainThreadId](const gusc::Threads::Thread::StopToken&) {
            mock.call();
            EXPECT_NE(mainThreadId, std::this_thread::get_id());
        };
        gusc::Threads::Thread t(local);
        t.start();
    }
    {
        // Anonymous copyable
        auto mainThreadId = std::this_thread::get_id();
        gusc::Threads::Thread t([mainThreadId](const gusc::Threads::Thread::StopToken&) {
            mock.call();
            EXPECT_NE(mainThreadId, std::this_thread::get_id());
        });
        t.start();
    }
    {
        // Anonymous movable
        auto mainThreadId = std::this_thread::get_id();
        std::unique_ptr<std::vector<int>> local_movable = std::make_unique<std::vector<int>>(100, 0);
        gusc::Threads::Thread t([mainThreadId, m=std::move(local_movable)](const gusc::Threads::Thread::StopToken&) {
            mock.call();
            EXPECT_NE(mainThreadId, std::this_thread::get_id());
        });
        t.start();
    }

    mock.setMock(nullptr);
}

TEST(ThreadTests, Functors)
{
    ThreadMock actualMock;
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(9);

    {
        functorWithToken fn;
        gusc::Threads::Thread t(fn);
        t.start();
    }
    {
        const functorWithToken fn;
        gusc::Threads::Thread t(fn);
        t.start();
    }
    {
        // Temporary struct
        gusc::Threads::Thread t(functorWithToken{});
        t.start();
    }
    {
        functorVoid fn;
        gusc::Threads::Thread t(fn);
        t.start();
    }
    {
        const functorVoid fn;
        gusc::Threads::Thread t(fn);
        t.start();
    }
    {
        // Temporary struct
        gusc::Threads::Thread t(functorVoid{});
        t.start();
    }
    {
        functorArgs fn;
        gusc::Threads::Thread t(fn, std::this_thread::get_id());
        t.start();
    }
    {
        const functorArgs fn;
        gusc::Threads::Thread t(fn, std::this_thread::get_id());
        t.start();
    }
    {
        // Temporary struct
        gusc::Threads::Thread t(functorArgs{}, std::this_thread::get_id());
        t.start();
    }

    mock.setMock(nullptr);
}

TEST(ThreadTests, Methods)
{
    ThreadMock actualMock;
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(10);

    {
        cls o;
        gusc::Threads::Thread t(std::bind(&cls::fnWithToken, &o, std::placeholders::_1));
        t.start();
    }
    {
        const cls o;
        gusc::Threads::Thread t(std::bind(&cls::fnConstWithToken, &o, std::placeholders::_1));
        t.start();
    }
    {
        // Static method
        gusc::Threads::Thread t(&cls::fnStaticWithToken);
        t.start();
    }
    {
        cls o;
        gusc::Threads::Thread t(std::bind(&cls::fnVoid, &o));
        t.start();
    }
    {
        const cls o;
        gusc::Threads::Thread t(std::bind(&cls::fnConstVoid, &o));
        t.start();
    }
    {
        // Static method
        gusc::Threads::Thread t(&cls::fnStaticVoid);
        t.start();
    }
    {
        cls o;
        gusc::Threads::Thread t(std::bind(&cls::fnArgs, &o, std::placeholders::_1, std::placeholders::_2), std::this_thread::get_id());
        t.start();
    }
    {
        cls o;
        auto movable_data = std::make_unique<std::vector<int>>(40, 0);
        // Because of the Thread's ability to be started and stopped we can not pass movable objects as thread procedure's arguments
        // as they will be moved away from internal strucutre on the first run, to avoid confusion this use-case has been removed
//        gusc::Threads::Thread t(std::bind(&cls::fnMoveArgs, &o, std::placeholders::_1, std::placeholders::_2), std::move(movable_data));
//        t.start();
        o.fnMoveArgs({}, std::move(movable_data));
    }
    {
        cls o;
        auto referable_data = std::make_unique<std::vector<int>>(40, 0);
        gusc::Threads::Thread t(std::bind(&cls::fnRefArgs, &o, std::placeholders::_1, std::placeholders::_2), std::ref(referable_data));
        t.start();
    }
    {
        cls o;
        auto referable_data = std::make_unique<std::vector<int>>(40, 0);
        gusc::Threads::Thread t(std::bind(&cls::fnConstRefArgs, &o, std::placeholders::_1, std::placeholders::_2), std::ref(referable_data));
        t.start();
    }

    mock.setMock(nullptr);
}


TEST(ThreadTests, ThisThread)
{
    ThreadMock actualMock;
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(1);

    auto mainThreadId = std::this_thread::get_id();
    gusc::Threads::ThisThread tt;
    tt.setThreadProcedure([&](const gusc::Threads::Thread::StopToken& token){
        tt.stop();
        if (token.getIsStopping())
        {
            mock.call();
        }
        EXPECT_EQ(mainThreadId, std::this_thread::get_id());
    });
    tt.start();

    mock.setMock(nullptr);
}

TEST(ThreadTests, ThreadPool)
{
    ThreadMock actualMock;
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(17);

    auto mainThreadId = std::this_thread::get_id();
    auto tp = gusc::Threads::ThreadPool(2, [&](const gusc::Threads::Thread::StopToken& token){
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
    });
    tp.start();
    std::this_thread::sleep_for(200ms);
    tp.stop();
    tp.resize(10);
    tp.start();
    std::this_thread::sleep_for(200ms);
    tp.stop();
    tp.resize(5);
    tp.start();
    std::this_thread::sleep_for(200ms);
    tp.stop();

    mock.setMock(nullptr);
}
    
TEST(ThreadTests, ThreadStartStop)
{
    ThreadMock actualMock;
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(2);

    auto mainThreadId = std::this_thread::get_id();
    gusc::Threads::Thread t([&](const gusc::Threads::Thread::StopToken& token){
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
        while (!token.getIsStopping())
        {
            std::this_thread::yield();
        }
    });
    t.start();
    std::this_thread::sleep_for(200ms);
    t.stop();
    t.join();
    t.start();
    std::this_thread::sleep_for(200ms);
    t.stop();
    t.join();

    mock.setMock(nullptr);
}
