//
//  TaskQueueTests.cpp
//  Threads
//
//  Created by Gusts Kaksis on 11/11/2022.
//  Copyright © 2022 Gusts Kaksis. All rights reserved.
//

#if defined(_WIN32)
#   include <Windows.h>
#endif

#include <gtest/gtest.h>

#include "Utilities.hpp"
#include "Threads/TaskQueue.hpp"
#include "TaskQueueMocks.hpp"

#include <chrono>

using namespace std::chrono_literals;

class SerialTaskQueueTest : public Test
{
public:
    TaskQueueMock actualMock;
    gusc::Threads::SerialTaskQueue queue { "SerialQueue" };
    std::mutex mutex;
};

class ParallelTaskQueueTest : public Test
{
public:
    TaskQueueMock actualMock;
    gusc::Threads::ParallelTaskQueue queue { "ParallelQueue", 4 };
    std::mutex mutex;
};

class TaskQueueOnThisThreadTest : public Test
{
public:
    TaskQueueMock actualMock;
    gusc::Threads::ThisThread tt;
    gusc::Threads::SerialTaskQueue queue { tt };
    std::mutex mutex;
};


TEST_F(SerialTaskQueueTest, Send)
{
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(14);

    auto mainThreadId = std::this_thread::get_id();

    queue.send(&fn);
    queue.send(lambda);
    queue.send(lambdaConst);
    queue.send(lambdaCopyCapture);
    // A lambda stored in a variable will be passed as reference which
    // results in a copy that again won't work because the object is moveable.
    // The workaround is to pass it as a reference wrapper (@note use only if
    // you are sure the reference will outlive the task queue!)
    queue.send(std::ref(lambdaMoveCapture));
    {
        functor fn;
        queue.send(fn);
    }
    {
        const functor fn;
        queue.send(fn);
    }
    // Temporary struct
    queue.send(functor{});
    {
        cls o;
        queue.send(std::bind(&cls::fn, &o));
        queue.send(std::bind(&cls::fnConst, &o));
        queue.send(&cls::fnStatic);
        // TODO: We should wait for the task to finish otherwise it might get executed after o is detroyed
    }
    auto localLambda = [&](){
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
    };
    queue.send(localLambda);
    queue.send([&](){
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
    });
    auto movable_data = std::make_unique<std::vector<int>>(10, 0);
    queue.send([mainThreadId, m=std::move(movable_data)](){
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
    });

    // Just wait till all the tasks are finished
    std::this_thread::sleep_for(100ms);

    mock.setMock(nullptr);
}

TEST_F(SerialTaskQueueTest, SendDelayed)
{
    mock.setMock(&actualMock);
    int numCalls { 1 };
    EXPECT_CALL(actualMock, call()).Times(numCalls);

    std::condition_variable cv;
    std::unique_lock lock { mutex };
    int counter { 0 };
    queue.sendDelayed([&](){
        mock.call();
        std::lock_guard lock { mutex };
        ++counter;
        cv.notify_one();
    }, 1s);
    auto result = cv.wait_for(lock, 1100ms, [&](){ return counter == numCalls; });
    EXPECT_TRUE(result);
    EXPECT_EQ(counter, numCalls);

    mock.setMock(nullptr);
}

TEST_F(SerialTaskQueueTest, Exceptions)
{
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(1);
    EXPECT_CALL(actualMock, noCall()).Times(0);
    EXPECT_CALL(actualMock, recover()).Times(1);

    try
    {
        queue.sendWait([](){
            mock.call();
            throw std::runtime_error("Oops");
            mock.noCall();
        });
    }
    catch (const std::exception& ex)
    {
        mock.recover();
    }

    mock.setMock(nullptr);
}

TEST_F(SerialTaskQueueTest, SendWait)
{
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(1);

    auto mainThreadId = std::this_thread::get_id();

    queue.sendWait([&](){
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
    });

    mock.setMock(nullptr);
}

TEST_F(SerialTaskQueueTest, SendAsync)
{
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(2);

    auto mainThreadId = std::this_thread::get_id();

    auto f1 = queue.sendAsync<int>([&]() -> int {
        // Try to do a blocking call from the same thread
        queue.sendWait([&](){
            mock.call();
            EXPECT_NE(mainThreadId, std::this_thread::get_id());
        });
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
        return 10;
    });
    auto f2 = queue.sendAsync<int>([&]() -> int {
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
        return 20;
    });

    EXPECT_EQ(f1.getValue(), 10);
    EXPECT_EQ(f2.getValue(), 20);

    mock.setMock(nullptr);
}

TEST_F(SerialTaskQueueTest, SendSync)
{
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(2);

    auto mainThreadId = std::this_thread::get_id();

    auto res1 = queue.sendSync<int>([&]() -> int {
        // Try to do a blocking call from the same thread
        queue.sendWait([&](){
            mock.call();
            EXPECT_NE(mainThreadId, std::this_thread::get_id());
        });
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
        return 1;
    });
    auto res2 = queue.sendSync<int>([&]() -> int {
        mock.call();
        EXPECT_NE(mainThreadId, std::this_thread::get_id());
        return 2;
    });

    EXPECT_EQ(res1, 1);
    EXPECT_EQ(res2, 2);

    mock.setMock(nullptr);
}

TEST_F(ParallelTaskQueueTest, SendAsync)
{
    mock.setMock(&actualMock);
    int numCalls { 4 };
    EXPECT_CALL(actualMock, call()).Times(numCalls);

    auto mainThreadId = std::this_thread::get_id();

    std::condition_variable cv;
    std::unique_lock lock { mutex };
    std::vector<std::thread::id> ids;

    const auto lambda = [&]() -> int {
        auto threadId = std::this_thread::get_id();
        EXPECT_NE(mainThreadId, threadId);
        // Wait for all other tasks to be picked up by different threads
        std::this_thread::sleep_for(100ms);
        // Test thread IDs
        std::lock_guard lock { mutex };
        auto it = std::find(ids.begin(), ids.end(), threadId);
        EXPECT_EQ(it, ids.end());
        ids.emplace_back(threadId);
        return 1;
    };
    auto f1 = queue.sendAsync<int>(std::ref(lambda));
    auto f2 = queue.sendAsync<int>(std::ref(lambda));
    auto f3 = queue.sendAsync<int>(std::ref(lambda));
    auto f4 = queue.sendAsync<int>(std::ref(lambda));

    auto res = cv.wait_for(lock, 200ms, [&](){ return ids.size() == numCalls; });
    EXPECT_TRUE(res);

    EXPECT_EQ(f1.getValue(), 1);
    EXPECT_EQ(f2.getValue(), 1);
    EXPECT_EQ(f3.getValue(), 1);
    EXPECT_EQ(f4.getValue(), 1);

    mock.setMock(nullptr);
}

TEST_F(TaskQueueOnThisThreadTest, Test)
{
    mock.setMock(&actualMock);
    EXPECT_CALL(actualMock, call()).Times(1);

    auto mainThreadId = std::this_thread::get_id();

    // Initiate ThisThread stop with a delay
    queue.sendDelayed([&](){
        tt.stop();
    }, 3s);

    queue.send([&](){
        mock.call();
        EXPECT_EQ(mainThreadId, std::this_thread::get_id());
    });

    // Start ThisThread
    tt.start();
}

