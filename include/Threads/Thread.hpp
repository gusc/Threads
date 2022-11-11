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
#include <mutex>
#include <queue>
#include <condition_variable>

namespace gusc::Threads
{

/// @brief Class representing a new thread
class Thread
{
public:
    Thread() = default;
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;
    virtual ~Thread()
    {
        setIsRunning(false);
        join();
    }
    
    /// @brief start the thread and it's run-loop
    virtual inline void start()
    {
        if (!getIsRunning())
        {
            setIsRunning(true);
            thread = std::make_unique<std::thread>(&Thread::runLoop, this);
        }
        else
        {
            throw std::runtime_error("Thread has already started");
        }
    }
    
    /// @brief signal the thread to stop - this also stops receiving new runnables
    /// @warning if a task is sent after calling this method an exception will be thrown
    virtual inline void stop()
    {
        if (getIsRunning())
        {
            setIsRunning(false);
        }
        else
        {
            throw std::runtime_error("Thread has not been started");
        }
    }
    
    /// @brief join the thread and wait unti it's finished
    inline void join()
    {
        if (thread && thread->joinable())
        {
            thread->join();
        }
    }
    
    /// @brief run a callable object on this thread
    /// @param newRunnable - any Callable object that will be executed on this thread
    template<typename TCallable>
    inline void run(const TCallable& newCallable)
    {
        if (getIsRunning())
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.emplace(std::make_unique<RunnableWithObject<TCallable>>(newCallable));
            queueWait.notify_one();
        }
        else
        {
            throw std::runtime_error("Thread is not running");
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
    inline bool operator==(const std::thread::id& other) const noexcept
    {
        return getId() == other;
    }
    inline bool operator!=(const std::thread::id& other) const noexcept
    {
        return !(operator==(other));
    }

    inline std::thread::id getId() const noexcept
    {
        if (thread)
        {
            return thread->get_id();
        }
        else
        {
            // We haven't started a thread yet, so we're the same thread
            return std::this_thread::get_id();
        }
    }
    
    inline bool getIsRunning() const noexcept
    {
        return isRunning;
    }
    
protected:
    inline void runLoop()
    {
        while (getIsRunning())
        {
            std::unique_ptr<Runnable> next;
            {
                std::unique_lock<decltype(mutex)> lock(mutex);
                if (!queue.empty())
                {
                    next = std::move(queue.front());
                    queue.pop();
                }
                else
                {
                    // Wait for any runnable to appear on the queue
                    queueWait.wait(lock);
                }
            }
            if (next)
            {
                next->run();
            }
        }
        runLeftovers();
    }
    
    inline void runLeftovers()
    {
        const std::lock_guard<decltype(mutex)> lock(mutex);
        while (queue.size())
        {
            queue.front()->run();
            queue.pop();
        }
    }
    
    inline void setIsRunning(bool newIsRunning) noexcept
    {
        isRunning = newIsRunning;
        queueWait.notify_one();
    }

private:
    
    /// @brief base class for thread runnable
    class Runnable
    {
    public:
        virtual ~Runnable() = default;
        virtual void run() {}
    };
    
    /// @brief templated task to wrap a callable object
    template<typename TRunnable>
    class RunnableWithObject : public Runnable
    {
    public:
        RunnableWithObject(const TRunnable& initRunnableObject)
            : runnableObject(initRunnableObject)
        {}
        inline void run() override
        {
            try
            {
                runnableObject();
            }
            catch(...)
            {
                // We can't do nothing as nobody is listening, but we don't want the thread to explode
            }
        }
    private:
        TRunnable runnableObject;
    };
    
    std::mutex mutex;
    std::atomic<bool> isRunning { false };
    std::queue<std::unique_ptr<Runnable>> queue;
    std::condition_variable queueWait;
    std::unique_ptr<std::thread> thread;
};

/// @brief Class representing a currently executing thread
class ThisThread : public Thread
{
public:
    ThisThread()
    {
        // ThisThread is already running
        setIsRunning(true);
    }
    
    /// @brief start the thread and it's run-loop
    /// @warning calling this method will efectivelly block current thread
    inline void start() override
    {
        setIsRunning(true);
        runLoop();
    }

    /// @brief signal the thread to stop - this also stops receiving tasks
    /// @warning if a task is sent after calling this method an exception will be thrown
    inline void stop() override
    {
        setIsRunning(false);
    }
};
    
}

#endif /* Thread_hpp */
