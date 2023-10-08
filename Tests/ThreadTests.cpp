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

#include "ThreadTests.hpp"
#include "Utilities.hpp"
#include "Threads/Thread.hpp"
#include "Threads/ThreadPool.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace
{
static Logger tlog;

std::shared_ptr<std::vector<int>> anon_copyable = std::make_shared<std::vector<int>>(50, 0);
std::unique_ptr<std::vector<int>> anon_movable = std::make_unique<std::vector<int>>(100, 0);

void f1(const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
}

void f2()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
}

auto l1 = [](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

const auto l2 = [](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

const auto l3 = [c=anon_copyable](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(c->size());
};

const auto l4 = [m=std::move(anon_movable)](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(m->size());
};

auto l5 = []()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

const auto l6 = []()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

}

static std::shared_ptr<std::vector<int>> stat_copyable = std::make_shared<std::vector<int>>(50, 0);
static std::unique_ptr<std::vector<int>> stat_movable = std::make_unique<std::vector<int>>(100, 0);

static void f3(const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
}

static void f4()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
}

static auto l7 = [](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

static const auto l8 = [](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

static const auto l9 = [c=stat_copyable](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(c->size());
};

static const auto l10 = [m=std::move(stat_movable)](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(m->size());
};

static auto l11 = []()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

static const auto l12 = []()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

struct s1
{
    void operator()(const gusc::Threads::Thread::StopToken&) const
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
    }
};

struct s2
{
    void operator()() const
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
    }
};

struct s3
{
    void operator()() const
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(data->size());
    }
    
    std::unique_ptr<std::vector<int>> data = std::make_unique<std::vector<int>>(10, 0);
};

class c1
{
public:
    void f1(const gusc::Threads::Thread::StopToken&)
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
    }
    
    void f2(const gusc::Threads::Thread::StopToken&) const
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
    }
    
    static void f3(const gusc::Threads::Thread::StopToken&)
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
    }
    
    void f4()
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
    }
    
    void f5() const
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
    }
    
    static void f6()
    {
        tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
    }
};

void runThreadTests()
{
    tlog << "Thread Tests";
    tlog << "Main thread ID: " + tidToStr(std::this_thread::get_id());

    gusc::Threads::Thread::StopToken token;
    
    // Test simple functions
    {
        f1(token);
        gusc::Threads::Thread t(&f1);
        t.start();
    }
    {
        f2();
        gusc::Threads::Thread t(&f2);
        t.start();
    }
    {
        f3(token);
        gusc::Threads::Thread t(&f3);
        t.start();
    }
    {
        f4();
        gusc::Threads::Thread t(&f4);
        t.start();
    }
    
    // Test lambdas
    {
        l1(token);
        gusc::Threads::Thread t(l1);
        t.start();
    }
    {
        l2(token);
        gusc::Threads::Thread t(l2);
        t.start();
    }
    {
        l3(token);
        gusc::Threads::Thread t(l3);
        t.start();
    }
    {
        l4(token);
        gusc::Threads::Thread t(l4);
        t.start();
    }
    {
        l5();
        gusc::Threads::Thread t(l5);
        t.start();
    }
    {
        l6();
        gusc::Threads::Thread t(l6);
        t.start();
    }
    {
        l7(token);
        gusc::Threads::Thread t(l7);
        t.start();
    }
    {
        l8(token);
        gusc::Threads::Thread t(l8);
        t.start();
    }
    {
        l9(token);
        gusc::Threads::Thread t(l9);
        t.start();
    }
    {
        l10(token);
        gusc::Threads::Thread t(l10);
        t.start();
    }
    {
        l11();
        gusc::Threads::Thread t(l11);
        t.start();
    }
    {
        l12();
        gusc::Threads::Thread t(l12);
        t.start();
    }
    {
        tlog << "Local lambda";
        auto local = [](const gusc::Threads::Thread::StopToken&) {
            tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
        };
        local(token);
        gusc::Threads::Thread t(local);
        t.start();
    }
    {
        // Anonymous copyable
        tlog << "Annonymous copyable lambda";
        gusc::Threads::Thread t([](const gusc::Threads::Thread::StopToken&) {
            tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
        });
        t.start();
    }
    {
        // Anonymous movable
        tlog << "Annonymous movable lambda";
        std::unique_ptr<std::vector<int>> local_movable = std::make_unique<std::vector<int>>(100, 0);
        gusc::Threads::Thread t([m=std::move(local_movable)](const gusc::Threads::Thread::StopToken&) {
            tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(m->size());
        });
        t.start();
    }
    
    // Test callable structs
    {
        s1 cb;
        cb(token);
        gusc::Threads::Thread t(cb);
        t.start();
    }
    {
        const s1 cb;
        cb(token);
        gusc::Threads::Thread t(cb);
        t.start();
    }
    {
        // Temporary struct
        tlog << "Temporary s1";
        gusc::Threads::Thread t(s1{});
        t.start();
    }
    {
        s2 cb;
        cb();
        gusc::Threads::Thread t(cb);
        t.start();
    }
    {
        const s2 cb;
        cb();
        gusc::Threads::Thread t(cb);
        t.start();
    }
    {
        // Temporary struct
        tlog << "Temporary s2";
        gusc::Threads::Thread t(s2{});
        t.start();
    }
    
    // Test object method
    {
        c1 o;
        o.f1(token);
        gusc::Threads::Thread t(std::bind(&c1::f1, &o, std::placeholders::_1));
        t.start();
    }
    {
        c1 o;
        o.f1(token);
        gusc::Threads::Thread t([&](const gusc::Threads::Thread::StopToken& tk) {
            o.f1(tk);
        });
        t.start();
    }
    {
        c1 o;
        o.f2(token);
        gusc::Threads::Thread t(std::bind(&c1::f2, &o, std::placeholders::_1));
        t.start();
    }
    {
        c1 o;
        o.f2(token);
        gusc::Threads::Thread t([&](const gusc::Threads::Thread::StopToken& tk) {
            o.f2(tk);
        });
        t.start();
    }
    {
        c1::f3(token);
        gusc::Threads::Thread t(&c1::f3);
        t.start();
    }
    {
        c1 o;
        o.f4();
        gusc::Threads::Thread t(std::bind(&c1::f4, &o));
        t.start();
    }
    {
        c1 o;
        o.f4();
        gusc::Threads::Thread t([&](const gusc::Threads::Thread::StopToken&) {
            o.f4();
        });
        t.start();
    }
    {
        c1 o;
        o.f5();
        gusc::Threads::Thread t(std::bind(&c1::f5, &o));
        t.start();
    }
    {
        c1 o;
        o.f5();
        gusc::Threads::Thread t([&](const gusc::Threads::Thread::StopToken&) {
            o.f5();
        });
        t.start();
    }
    {
        c1::f6();
        gusc::Threads::Thread t(&c1::f6);
        t.start();
    }
    
    
    // Test ThisThread
    {
        gusc::Threads::ThisThread tt;
        tt.setThreadProcedure([&](const gusc::Threads::Thread::StopToken& token){
            tlog << "Anonymous lambda thread ID: " + tidToStr(std::this_thread::get_id());
            tt.stop();
            if (token.getIsStopping())
            {
                tlog << "This thread has been informed to stop";
            }
        });
        tt.start();
    }
    
    // Test ThreadPool
    {
        auto tp = gusc::Threads::ThreadPool(2, [&](const gusc::Threads::Thread::StopToken& token){
            tlog << "Running on a thread pool thread ID: " + tidToStr(std::this_thread::get_id());
        });
    }
    
    // Test start/stop
    {
        gusc::Threads::Thread t([](const gusc::Threads::Thread::StopToken&){
            tlog << "Starting thread ID: " + tidToStr(std::this_thread::get_id());
        });
        t.start();
        t.stop();
        t.join();
        t.start();
        t.stop();
        t.join();
    }
    
    tlog.flush();
}
