//
//  Thread.hpp
//  Threads
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#ifndef GUSC_THREAD_HPP
#define GUSC_THREAD_HPP

#include <thread>
#include <atomic>
#include <future>
#include <type_traits>

#if !defined(_WIN32)
#   include <pthread.h>
#   if defined(__APPLE__)
#       include <mach/mach_time.h>
#       include <mach/thread_policy.h>
#       include <mach/thread_act.h>
#   endif
#endif

#include "private/Utilities.hpp"

namespace gusc
{

namespace Threads
{

/// @brief Class representing a new thread with delayed start
class Thread
{
public:
    /// @brief Thread priorities
    /// Default - leave the default priority that is implementation and platform specific
    /// RealTime - elevate priority to highest possible (real-time threads on macOS and iOS, THREAD_PRIORITY_TIME_CRITICAL on Windows)
    enum class Priority
    {
        Default,
        Low,
        High,
        RealTime
    };

    /// @brief an object returned from Thread::start() method that informs when thread has fully initialized
    /// Use this token to pause caller thread until this thread has fully started
    class StartToken
    {
        friend Thread;
    public:
        StartToken() = default;
        StartToken(std::future<void> initFuture)
            : future(std::move(initFuture))
        {}
        StartToken(StartToken&& other) noexcept
            : isStarted(other.isStarted.load())
            , future(std::move(other.future))
        {}
        StartToken& operator=(StartToken&& other) noexcept
        {
            isStarted = other.isStarted.load();
            future = std::move(other.future);
            return *this;
        }
        inline bool getIsStarted() const noexcept
        {
            return isStarted;
        }
        inline void wait()
        {
            future.wait();
        }
    private:
        std::atomic_bool isStarted { false };
        std::future<void> future;
    };
    
    /// @brief an object passed to thread procedure to notify it about thread being stopped
    /// Use this token in your thread run-loop to detecect when thread is being stopped and
    /// do a graceful exit
    class StopToken
    {
        friend Thread;
    public:
        StopToken() = default;
        StopToken(StopToken&& other) noexcept
            : isStopping(other.isStopping.load())
        {}
        StopToken& operator=(StopToken&& other) noexcept
        {
            isStopping = other.isStopping.load();
            return *this;
        }
        inline bool getIsStopping() const noexcept
        {
            return isStopping;
        }
    private:
        std::atomic_bool isStopping { false };
        
        inline void notifyStop() noexcept
        {
            isStopping = true;
        }
    };

    Thread() = default;
    // Thread("name", priority, function, args...)
    template <class TFn, class ...TArgs>
    Thread(const std::string& initThreadName, Priority initPriority, TFn&& fn, TArgs&&... args)
        : threadName(initThreadName)
        , threadProcedure(std::forward<TFn>(fn), std::forward<TArgs>(args)...)
        , priority(initPriority)
    {}
    // Thread("name", function, args...)
    template <class TFn, class ...TArgs>
    Thread(const std::string& name, TFn&& fn, TArgs&&... args)
        : Thread(name, Priority::Default, std::forward<TFn>(fn), std::forward<TArgs>(args)...)
    {}
    // Thread(priority, function, args...)
    template <class TFn, class ...TArgs>
    Thread(Priority priority, TFn&& fn, TArgs&&... args)
        : Thread("gust::Threads::Thread", priority, std::forward<TFn>(fn), std::forward<TArgs>(args)...)
    {}
    // Thread(function, args...)
    template <class TFn, class ...TArgs,
        class = typename std::enable_if<
            !IsSameType<TFn, std::string>::value &&
            !IsSameType<TFn, char*>::value &&
            !IsSameType<TFn, Priority>::value
        >::type
    >
    explicit Thread(TFn&& fn, TArgs&&... args)
        : Thread("gust::Threads::Thread", Priority::Default, std::forward<TFn>(fn), std::forward<TArgs>(args)...)
    {}

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;
    virtual ~Thread()
    {
        try
        {
            if (getIsStarted())
            {
                stopToken.notifyStop();
            }
            join();
        }
        catch (...)
        {
        }
    }
    
    /// @brief start the thread
    /// @returns start token you can use to wait for the thread routine to actually start
    virtual inline StartToken& start()
    {
        if (!getIsStarted())
        {
            setIsStarted(true);
            createStartToken();
            // If the thread was already constructed previously we need to detach it first otherwise it will throw
            if (thread.joinable())
            {
                thread.detach();
            }
            thread = std::thread{ &Thread::run, this };
            setThreadPriority();
            setThreadName();
            return startToken;
        }
        else
        {
            throw std::runtime_error("Thread has already started");
        }
    }
    
    /// @brief signal the thread to stop
    virtual inline void stop()
    {
        if (getIsStarted())
        {
            stopToken.notifyStop();
        }
    }
    
    /// @brief join the thread and wait unti it's finished
    virtual inline void join()
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
        
    inline bool operator==(const Thread& other) const noexcept
    {
        return getId() == other.getId();
    }
    inline bool operator!=(const Thread& other) const noexcept
    {
        return !(operator==(other));
    }
    inline bool operator==(const std::thread& other) const noexcept
    {
        return getId() == other.get_id();
    }
    inline bool operator!=(const std::thread& other) const noexcept
    {
        return !(operator==(other));
    }
    inline bool operator==(const std::thread::id& other) const noexcept
    {
        return getId() == other;
    }
    inline bool operator!=(const std::thread::id& other) const noexcept
    {
        return !(operator==(other));
    }

    virtual inline std::thread::id getId() const noexcept
    {
        return thread.get_id();
    }
    
    inline bool getIsStarted() const noexcept
    {
        return isStarted;
    }
    
    inline bool getIsStopping() const noexcept
    {
        return stopToken.getIsStopping();
    }
    
protected:
    inline void run() noexcept
    {
        setThisThreadPriority();
        setThisThreadName();
        startToken.isStarted = true;
        try
        {
            startPromise.set_value();
            threadProcedure(stopToken);
        }
        catch (...)
        {
            // Prevent the thread from crashing
        }
        // The thread procedure has finished so the thread will stop now
        setIsStarted(false);
    }
    
    inline void setIsStarted(bool newIsStarted) noexcept
    {
        isStarted = newIsStarted;
    }
    
    inline StartToken& createStartToken() noexcept
    {
        startPromise = {};
        startToken = { startPromise.get_future() };
        return startToken;
    }
    
    inline void createStopToken() noexcept
    {
        stopToken = StopToken{};
    }
    
    template <class TFn, class ...TArgs>
    inline void changeThreadProcedure(TFn&& fn, TArgs&&... args)
    {
        if (!getIsStarted())
        {
            threadProcedure = ThreadProcedure{ std::forward<TFn>(fn), std::forward<TArgs>(args)... };
        }
        else
        {
            throw std::runtime_error("Can not change the runnable of the thread that has already started");
        }
    }

#if defined(__APPLE__)
#   include "private/ThreadApple.hpp"
#elif defined(_WIN32)
#   include "private/ThreadWindows.hpp"
#else
#   include "private/ThreadLinux.hpp"
#endif
    
private:
    #include "private/ThreadStructures.hpp"
    
    std::atomic_bool isStarted { false };
    std::promise<void> startPromise;
    StartToken startToken;
    StopToken stopToken;
    std::string threadName { "gust::Threads::Thread" };
    ThreadProcedure threadProcedure;
    Priority priority { Priority::Default };
    std::thread thread;
};

/// @brief Class representing a currently executing thread
class ThisThread final : public Thread
{
public:
    /// @brief set a thread procedure to use on this thread with or without StopToken
    template <class TFn, class ...TArgs>
    inline void setThreadProcedure(TFn&& fn, TArgs&&... args)
    {
        changeThreadProcedure(std::forward<TFn>(fn), std::forward<TArgs>(args)...);
    }
    /// @brief start the thread and it's run-loop
    /// @warning calling this method will efectivelly block current thread
    inline StartToken& start() override
    {
        if (!getIsStarted())
        {
            setIsStarted(true);
            createStopToken();
            auto& token = createStartToken();
            run();
            return token;
        }
        else
        {
            throw std::runtime_error("Thread has already started");
        }
    }
    
    inline std::thread::id getId() const noexcept override
    {
        return std::this_thread::get_id();
    }
    
    inline void join() override
    {}
};
    
}
} // namespace gusc::Threads

#endif /* GUSC_THREAD_HPP */
