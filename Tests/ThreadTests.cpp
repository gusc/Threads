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

void af1(const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
}

void af2()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
}

void af3(const gusc::Threads::Thread::StopToken&, const std::string& additionalArg)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << additionalArg;
}

auto al1 = [](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

const auto al2 = [](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

const auto al3 = [c=anon_copyable](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(c->size());
};

const auto al4 = [m=std::move(anon_movable)](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(m->size());
};

auto al5 = []()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

const auto al6 = []()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

auto al7 = [](const gusc::Threads::Thread::StopToken&, const std::string& additionalArg)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << additionalArg;
};

}

static std::shared_ptr<std::vector<int>> stat_copyable = std::make_shared<std::vector<int>>(50, 0);
static std::unique_ptr<std::vector<int>> stat_movable = std::make_unique<std::vector<int>>(100, 0);

static void sf1(const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
}

static void sf2()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
}

static void sf3(const gusc::Threads::Thread::StopToken&, const std::string& additionalArg)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << additionalArg;
}


static auto sl1 = [](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

static const auto sl2 = [](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

static const auto sl3 = [c=stat_copyable](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(c->size());
};

static const auto sl4 = [m=std::move(stat_movable)](const gusc::Threads::Thread::StopToken&)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << std::to_string(m->size());
};

static auto sl5 = []()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

static const auto sl6 = []()
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id());
};

static auto sl7 = [](const gusc::Threads::Thread::StopToken&, const std::string& additionalArg)
{
    tlog << "Function: " <<  __func__ << " ID: " << tidToStr(std::this_thread::get_id()) << " X: " << additionalArg;
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
        af1(token);
        gusc::Threads::Thread t(&af1);
        t.start();
    }
    {
        af2();
        gusc::Threads::Thread t(&af2);
        t.start();
    }
    {
        af3(token, "string");
        gusc::Threads::Thread t(&af3, std::string("string"));
        t.start();
    }
    {
        sf1(token);
        gusc::Threads::Thread t(&sf1);
        t.start();
    }
    {
        sf2();
        gusc::Threads::Thread t(&sf2);
        t.start();
    }
    {
        sf3(token, "string");
        gusc::Threads::Thread t(&sf3, std::string("string"));
        t.start();
    }
    
    // Test lambdas
    {
        al1(token);
        gusc::Threads::Thread t(al1);
        t.start();
    }
    {
        al2(token);
        gusc::Threads::Thread t(al2);
        t.start();
    }
    {
        al3(token);
        gusc::Threads::Thread t(al3);
        t.start();
    }
    {
        al4(token);
        gusc::Threads::Thread t(al4);
        t.start();
    }
    {
        al5();
        gusc::Threads::Thread t(al5);
        t.start();
    }
    {
        al6();
        gusc::Threads::Thread t(al6);
        t.start();
    }
    {
        al7(token, "string");
        gusc::Threads::Thread t(al7, std::string("string"));
        t.start();
    }
    {
        sl1(token);
        gusc::Threads::Thread t(sl1);
        t.start();
    }
    {
        sl2(token);
        gusc::Threads::Thread t(sl2);
        t.start();
    }
    {
        sl3(token);
        gusc::Threads::Thread t(sl3);
        t.start();
    }
    {
        sl4(token);
        gusc::Threads::Thread t(sl4);
        t.start();
    }
    {
        sl5();
        gusc::Threads::Thread t(sl5);
        t.start();
    }
    {
        sl6();
        gusc::Threads::Thread t(sl6);
        t.start();
    }
    {
        sl7(token, "string");
        gusc::Threads::Thread t(sl7, std::string("string"));
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
