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

#include <chrono>

using namespace std::chrono_literals;

namespace
{
static Logger tlog;

std::shared_ptr<std::vector<int>> anon_copyable = std::make_shared<std::vector<int>>(50, 0);
std::unique_ptr<std::vector<int>> anon_movable = std::make_unique<std::vector<int>>(100, 0);

void af1()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
}

auto al1 = []()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
};

const auto al2 = []()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
};

const auto al3 = [c=anon_copyable]()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id()) + " X:" + std::to_string(c->size());
};

const auto al4 = [m=std::move(anon_movable)]()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id()) + " X:" + std::to_string(m->size());
};

}

static std::shared_ptr<std::vector<int>> stat_copyable = std::make_shared<std::vector<int>>(50, 0);
static std::unique_ptr<std::vector<int>> stat_movable = std::make_unique<std::vector<int>>(100, 0);

static void sf1()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
}

static auto sl1 = []()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
};

static const auto sl2 = []()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
};

static const auto sl3 = [c=stat_copyable]()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id()) + " X:" + std::to_string(c->size());
};

static const auto sl4 = [m=std::move(stat_movable)]()
{
    tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id()) + " X:" + std::to_string(m->size());
};

struct s1
{
    void operator()() const
    {
        tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
    }
};

class c1
{
public:
    void f1()
    {
        tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
    }
    
    void f2() const
    {
        tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
    }
    
    static void f3()
    {
        tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
    }
};

TEST(ThreadTests, TaskQueue)
{
    tlog << "========================";
    tlog << "Task Queue Tests";
    tlog << "Main thread ID: " + tidToStr(std::this_thread::get_id());
    tlog << "========================";
    
    gusc::Threads::ThisThread tt;
    
    gusc::Threads::SerialTaskQueue t1;
    gusc::Threads::SerialTaskQueue t2{tt};
    gusc::Threads::ParallelTaskQueue t3("parallel", 4);

    // Test regular sends
    
    af1();
    t1.send(&af1);
    t2.send(&af1);
    t3.send(&af1);
    t3.send(&af1);
    
    al1();
    t1.send(al1);
    t2.send(al1);
    t3.send(al1);
    t3.send(al1);
    
    al2();
    t1.send(al2);
    t2.send(al2);
    t3.send(al2);
    t3.send(al2);
    
    al3();
    t1.send(al3);
    t2.send(al3);
    t3.send(al3);
    t3.send(al3);
    
    al4();
    // A lambda stored in a variable will be passed as reference which
    // results in a copy that again won't work because the object is moveable.
    // The workaround is to pass it as a reference wrapper (@note use only if
    // you are sure the reference will outlive the task queue!)
    t1.send(std::ref(al4));
    t2.send(std::ref(al4));
    t3.send(std::ref(al4));
    t3.send(std::ref(al4));
    
    sf1();
    t1.send(&sf1);
    t2.send(&sf1);
    t3.send(&sf1);
    t3.send(&sf1);
    
    sl1();
    t1.send(sl1);
    t2.send(sl1);
    t3.send(sl1);
    t3.send(sl1);
    
    sl2();
    t1.send(sl2);
    t2.send(sl2);
    t3.send(sl2);
    t3.send(sl2);
    
    sl3();
    t1.send(sl3);
    t2.send(sl3);
    t3.send(sl3);
    t3.send(sl3);
    
    sl4();
    // A lambda stored in a variable will be passed as reference which
    // results in a copy that again won't work because the object is moveable.
    // The workaround is to pass it as a reference wrapper (@note use only if
    // you are sure the reference will outlive the task queue!)
    t1.send(std::ref(sl4));
    t2.send(std::ref(sl4));
    t3.send(std::ref(sl4));
    t3.send(std::ref(sl4));
    
    {
        s1 cb;
        cb();
        t1.send(cb);
        t2.send(cb);
        t3.send(cb);
        t3.send(cb);
    }
    {
        const s1 cb;
        cb();
        t1.send(cb);
        t2.send(cb);
        t3.send(cb);
        t3.send(cb);
    }
    {
        // Temporary struct
        tlog << "Temporary s1";
        t1.send(s1{});
        t2.send(s1{});
        t3.send(s1{});
        t3.send(s1{});
    }
    
    {
        c1 o;
        o.f1();
        t1.send(std::bind(&c1::f1, &o));
        t2.send(std::bind(&c1::f1, &o));
        t3.send(std::bind(&c1::f1, &o));
        t3.send(std::bind(&c1::f1, &o));
    }
    {
        c1 o;
        o.f2();
        t1.send(std::bind(&c1::f2, &o));
        t2.send(std::bind(&c1::f2, &o));
        t3.send(std::bind(&c1::f2, &o));
        t3.send(std::bind(&c1::f2, &o));
    }
    {
        c1::f3();
        t1.send(&c1::f3);
        t2.send(&c1::f3);
        t3.send(&c1::f3);
        t3.send(&c1::f3);
    }
    
    {
        tlog << "Local lambda";
        auto localLambda = [](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
        };
        localLambda();
        t1.send(localLambda);
        t2.send(localLambda);
        t3.send(localLambda);
        t3.send(localLambda);
    }
    {
        tlog << "Anonymous lambda";
        t1.send([](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
        });
        t2.send([](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
        });
        t3.send([](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
        });
        t3.send([](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id());
        });
    }
    {
        tlog << "Anonymous lambda with movable data";
        auto movable_data1 = std::make_unique<std::vector<int>>(10, 0);
        t1.send([m=std::move(movable_data1)](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id()) + " X: " + std::to_string(m->size());
        });
        auto movable_data2 = std::make_unique<std::vector<int>>(10, 0);
        t2.send([m=std::move(movable_data2)](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id()) + " X: " + std::to_string(m->size());
        });
        auto movable_data3 = std::make_unique<std::vector<int>>(10, 0);
        t3.send([m=std::move(movable_data3)](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id()) + " X: " + std::to_string(m->size());
        });
        auto movable_data4 = std::make_unique<std::vector<int>>(10, 0);
        t3.send([m=std::move(movable_data4)](){
            tlog << "Function: " + std::string(__func__) + " ID: " + tidToStr(std::this_thread::get_id()) + " X: " + std::to_string(m->size());
        });
    }
    
    // Test delayed tasks
    t1.sendDelayed([](){
        tlog << "Delayed message on thread ID: " + tidToStr(std::this_thread::get_id());
        tlog.flush();
    }, 1s);
    t2.sendDelayed([](){
        tlog << "Delayed message on thread ID: " + tidToStr(std::this_thread::get_id());
        tlog.flush();
    }, 2s);
    t3.sendDelayed([](){
        tlog << "Delayed message on thread ID: " + tidToStr(std::this_thread::get_id());
        tlog.flush();
    }, 1s);
    t3.sendDelayed([](){
        tlog << "Delayed message on thread ID: " + tidToStr(std::this_thread::get_id());
        tlog.flush();
    }, 2s);
    
    std::this_thread::sleep_for(2.5s);
    
    // Test exceptions
    try
    {
        t1.sendWait([](){
            tlog << "Exception lambda thread ID: " + tidToStr(std::this_thread::get_id());
            throw std::runtime_error("Oops");
        });
        t3.sendWait([](){
            tlog << "Exception lambda thread ID: " + tidToStr(std::this_thread::get_id());
            throw std::runtime_error("Oops");
        });
    }
    catch (const std::exception& ex)
    {
        tlog << ex.what();
    }
    
    // Test blocking calls
    t1.sendWait([](){
        tlog << "Blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t2.sendWait([](){
        tlog << "Blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t3.sendWait([](){
        tlog << "Blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    t3.sendWait([](){
        tlog << "Blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
    });
    auto f1 = t1.sendAsync<int>([&]() -> int {
        // Try to do a blocking call from the same thread
        t1.sendWait([](){
            tlog << "Sub blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
        });
        return 10;
    });
    auto f2 = t2.sendAsync<int>([]() -> int {
        return 20;
    });
    auto f3 = t3.sendAsync<int>([&]() -> int {
        // Try to do a blocking call from the same thread
        t3.sendWait([](){
            tlog << "Sub blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
        });
        return 10;
    });
    auto f4 = t3.sendAsync<int>([]() -> int {
        return 20;
    });
    auto res1 = t1.sendSync<int>([&]() -> int {
        // Try to do a blocking call from the same thread
        t1.sendWait([](){
            tlog << "Sub blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
        });
        return 1;
    });
    auto res2 = t2.sendSync<int>([]() -> int {
        return 2;
    });
    auto res3 = t3.sendSync<int>([&]() -> int {
        // Try to do a blocking call from the same thread
        t3.sendWait([](){
            tlog << "Sub blocking lambda thread ID: " + tidToStr(std::this_thread::get_id());
        });
        return 1;
    });
    auto res4 = t3.sendSync<int>([]() -> int {
        return 2;
    });
    tlog << "Sync and async results" + std::to_string(f1.getValue()) + std::to_string(f2.getValue()) + std::to_string(res1) + std::to_string(res2);
    tlog.flush();
    
    t2.sendDelayed([&](){
        tlog << "Stopping serial queue running on ThisThread";
        tt.stop();
    }, 3s);
    
    // Start ThisThread
    tt.start();
    std::this_thread::sleep_for(5s);
    
    // Task queue objects are destroyed and their threads are stopped
    
    tlog << "========================";
    tlog << "Done";
    tlog << "========================";
    
    tlog.flush();
}
