//
//  Thread.hpp
//  Threads
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#ifndef Thread_hpp
#define Thread_hpp

#include <thread>
#include <atomic>
#include <future>

#if !defined(_WIN32)
#   include <pthread.h>
#endif

namespace gusc
{
namespace Threads
{

/// @brief Class representing a new thread with delayed start
class Thread
{
public:
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
    Thread(const std::function<void(void)>& initThreadProcedure)
        : threadProcedure([initThreadProcedure](const StopToken&){
            initThreadProcedure();
        })
    {}
    Thread(const std::string& initThreadName,const std::function<void(void)>& initThreadProcedure)
        : threadProcedure([initThreadProcedure](const StopToken&){
            initThreadProcedure();
        })
        , threadName(initThreadName)
    {}
    Thread(const std::function<void(const StopToken&)>& initThreadProcedure)
        : threadProcedure(initThreadProcedure)
    {}
    Thread(const std::string& initThreadName, const std::function<void(const StopToken&)>& initThreadProcedure)
        : threadProcedure(initThreadProcedure)
        , threadName(initThreadName)
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
    virtual inline StartToken& start()
    {
        if (!getIsStarted())
        {
            setIsStarted(true);
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
        if (threadProcedure)
        {
            try
            {
                threadProcedure(stopToken);
            }
            catch (...)
            {
                // Prevent the thread from crashing
            }
        }
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
    
    inline void changeThreadProcedure(const std::function<void(const StopToken&)>& newThreadProcedure)
    {
        if (!getIsStarted())
        {
            threadProcedure = newThreadProcedure;
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
    std::atomic_bool isStarted { false };
    std::promise<void> startPromise;
    StartToken startToken;
    StopToken stopToken;
    std::function<void(const StopToken&)> threadProcedure;
    std::string threadName { "gust::Threads::Thread " };
    std::thread thread;
};

/// @brief Class representing a currently executing thread
class ThisThread : public Thread
{
public:
    /// @brief set a thread procedure to use on this thread with StopToken
    inline void setThreadProcedure(const std::function<void(const StopToken&)>& newProc)
    {
        changeThreadProcedure(newProc);
    }
    /// @brief set a thread procedure to use on this thread
    inline void setThreadProcedure(const std::function<void(void)>& newProc)
    {
        changeThreadProcedure([newProc](const StopToken&){
            newProc();
        });
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
            setIsStarted(false);
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

#endif /* Thread_hpp */
