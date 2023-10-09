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
#include "private/function_traits.hpp"

#if !defined(_WIN32)
#   include <pthread.h>
#endif

namespace gusc
{

template<class TA, class TB>
using IsSameType = std::is_same<
    typename std::decay<
        typename std::remove_cv<
            typename std::remove_reference<TA>::type
        >::type
    >::type,
    TB
>;

template<class TFn, std::size_t argN, class TArg>
using IsSameFunctionArgType = std::conjunction<
    typename function_traits<TFn>::template argExists<argN>,
    IsSameType<
        typename function_traits<TFn>::template argument<argN>::type,
        TArg
    >
>;

namespace Threads
{

/// @brief Class representing a new thread with delayed start
class Thread
{
public:
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
    // Thread("name", function, args...)
    template <class TFn, class ...TArgs>
    Thread(const std::string& initThreadName, TFn&& fn, TArgs&&... args)
        : threadName(initThreadName)
        , threadProcedure(std::forward<TFn>(fn), std::forward<TArgs>(args)...)
    {}
    // Thread(function, args...)
    template <class TFn, class ...TArgs,
        class = typename std::enable_if<
            !IsSameType<TFn, std::string>::value &&
            !IsSameType<TFn, char*>::value
        >::type
    >
    explicit Thread(TFn&& fn, TArgs&&... args)
        : Thread("gust::Threads::Thread", std::forward<TFn>(fn), std::forward<TArgs>(args)...)
    {}

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;
    virtual ~Thread()
    {
        if (getIsStarted())
        {
            stopToken.notifyStop();
            join();
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
        else
        {
            throw std::runtime_error("Thread has not been started");
        }
    }
    
    /// @brief join the thread and wait unti it's finished
    virtual inline void join()
    {
        if (getIsStarted() && thread.joinable())
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
    
protected:
    inline void run()
    {
#if defined(__APPLE__)
        if (!threadName.empty())
        {
            pthread_setname_np(threadName.c_str());
        }
#endif
        startToken.isStarted = true;
        startPromise.set_value();
        try
        {
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
    
    inline void setThreadName()
    {
#if !defined(__APPLE__)
        if (!threadName.empty())
        {
            auto threadHandle = thread.native_handle();
#   if defined(_WIN32)
            auto threadId = GetThreadId(threadHandle);
            // taken from https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2008/xcb2z8hs(v=vs.90)
            const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
            typedef struct tagTHREADNAME_INFO
            {
                DWORD dwType; // Must be 0x1000.
                LPCSTR szName; // Pointer to name (in user addr space).
                DWORD dwThreadID; // Thread ID (-1=caller thread).
                DWORD dwFlags; // Reserved for future use, must be zero.
            } THREADNAME_INFO;
#pragma pack(pop)
            THREADNAME_INFO Info;
            Info.dwType = 0x1000;
            Info.szName = threadName.c_str();
            Info.dwThreadID = threadId;
            Info.dwFlags = 0;
            __try
            {
                RaiseException(MS_VC_EXCEPTION, 0, sizeof(Info) / sizeof(ULONG_PTR), (ULONG_PTR*)&Info);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
#   elif defined(__linux__)
            pthread_setname_np(threadHandle, threadName.c_str());
#   endif
            // On Apple we can only set name of current thread (see run())
        }
#endif
    }
    
private:
    class CallableContainerBase
    {
    public:
        virtual ~CallableContainerBase() = default;
        virtual void call(const StopToken&) = 0;
    };
    
    template<class TFn, class ...TArgs>
    class CallableContainer : public CallableContainerBase
    {
    public:
        explicit CallableContainer(TFn&& initFn, TArgs&&... initArgs)
            : fn(std::forward<TFn>(initFn))
            , args(std::make_tuple<TArgs&&...>(std::forward<TArgs>(initArgs)...))
        {}
        
        void call(const StopToken& stopToken) override
        {
            callSpec<TFn>(stopToken, std::index_sequence_for<TArgs...>());
        }
        
    private:
        TFn fn;
        std::tuple<TArgs...> args;
        
        template<typename T, std::size_t... Is>
        inline
        typename std::enable_if_t<
            IsSameFunctionArgType<T, 0, StopToken>::value,
            void> callSpec(const StopToken& stopToken, std::index_sequence<Is...>)
        {
            fn(stopToken, std::get<Is>(args)...);
        }
        
        template<typename T, std::size_t... Is>
        inline
        typename std::enable_if_t<
            !IsSameFunctionArgType<T, 0, StopToken>::value,
            void> callSpec(const StopToken&, std::index_sequence<Is...>)
        {
            fn(std::get<Is>(args)...);
        }
    };
    
    class ThreadProcedure
    {
    public:
        ThreadProcedure() = default;
        
        template<class TFn, class ...TArgs>
        explicit ThreadProcedure(TFn&& fn, TArgs&&... args)
            : threadProcedure(std::make_unique<CallableContainer<TFn, TArgs...>>(std::forward<TFn>(fn), std::forward<TArgs>(args)...))
        {}
        
        inline void operator()(const StopToken& stopToken) const
        {
            if (threadProcedure)
            {
                threadProcedure->call(stopToken);
            }
        }
        
    private:
        std::unique_ptr<CallableContainerBase> threadProcedure;
    };
    
    std::atomic_bool isStarted { false };
    std::promise<void> startPromise;
    StartToken startToken;
    StopToken stopToken;
    std::string threadName { "gust::Threads::Thread" };
    ThreadProcedure threadProcedure;
    std::thread thread;
};

/// @brief Class representing a currently executing thread
class ThisThread : public Thread
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
