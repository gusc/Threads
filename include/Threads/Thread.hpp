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

namespace gusc::Threads
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
    Thread(const std::function<void(const StopToken&)>& initFunction)
        : runnable(initFunction)
    {}
    Thread(const std::function<void(void)>& initFunction)
        : runnable([initFunction](const StopToken&){initFunction();})
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
        startToken.isStarted = true;
        startPromise.set_value();
        if (runnable)
        {
            try
            {
                runnable(stopToken);
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
    
    inline void setRunnable(const std::function<void(const StopToken&)>& newRunnable)
    {
        if (!getIsStarted())
        {
            runnable = newRunnable;
        }
        else
        {
            throw std::runtime_error("Can not change the runnable of the thread that has already started");
        }
    }
    
private:
    std::atomic_bool isStarted { false };
    std::promise<void> startPromise;
    StartToken startToken;
    StopToken stopToken;
    std::function<void(const StopToken&)> runnable;
    std::thread thread;
};

/// @brief Class representing a currently executing thread
class ThisThread : public Thread
{
public:
    /// @brief set a thread procedure to use on this thread with StopToken
    inline void setThreadProc(const std::function<void(const StopToken&)>& newProc)
    {
        setRunnable(newProc);
    }
    /// @brief set a thread procedure to use on this thread
    inline void setThreadProc(const std::function<void(void)>& newProc)
    {
        setRunnable([newProc](const StopToken&){
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
    
} // namespace gusc::Threads

#endif /* Thread_hpp */
