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
        StartToken()
            : future(promise.get_future())
        {}
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
        std::promise<void> promise;
        std::future<void> future;
        
        inline void notifyStart() noexcept
        {
            isStarted = true;
            promise.set_value();
        }
    };
    
    class StopToken
    {
        friend Thread;
    public:
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
            setIsStarted();
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
    
protected:
    inline void run()
    {
        startToken.notifyStart();
        runnable(stopToken);
    }
    
    inline void setIsStarted() noexcept
    {
        isStarted = true;
    }
    
    inline bool getIsStarted() const noexcept
    {
        return isStarted;
    }
    
private:
    
    /// @brief base class for thread runnable
    class Runnable
    {
    public:
        Runnable(const std::function<void(const StopToken&)>& initFunction)
            : function(initFunction)
        {}
        inline void operator()(const StopToken& token) const
        {
            try
            {
                function(token);
            }
            catch(...)
            {
                // We can't do nothing as nobody is listening, but we don't want the thread to explode
            }
        }
    private:
        std::function<void(const StopToken&)> function;
    };
    
    std::atomic_bool isStarted { false };
    StartToken startToken;
    StopToken stopToken;
    Runnable runnable;
    std::thread thread;
};

/// @brief Class representing a currently executing thread
class ThisThread : public Thread
{
public:
    /// @brief start the thread and it's run-loop
    /// @warning calling this method will efectivelly block current thread
    inline StartToken& start() override
    {
        setIsStarted();
        run();
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
