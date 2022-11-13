//
//  ThreadPool.hpp
//  Threads
//
//  Created by Gusts Kaksis on 13/11/2022.
//  Copyright © 2022 Gusts Kaksis. All rights reserved.
//

#ifndef ThreadPool_hpp
#define ThreadPool_hpp

#include <thread>
#include <vector>
#include <atomic>

namespace gusc::Threads
{

///@brief Class representing a thread pool
class ThreadPool
{
public:
    ThreadPool(std::size_t initThreadCount)
        : threadCount(initThreadCount)
    {}
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    ~ThreadPool()
    {
        stop();
        for (std::size_t i = 0; i < threadCount; ++i)
        {
            if (threads.at(i).joinable())
            {
                threads.at(i).join();
            }
        }
        threads.clear();
    }
    
    inline void start()
    {
        if (!getIsRunning())
        {
            const std::lock_guard<decltype(mutex)> lock(mutex);
            threads.clear();
            threads.reserve(threadCount);
            for (std::size_t i = 0; i < threadCount; ++i)
            {
                threads.emplace_back(std::bind(&ThreadPool::runLoop, this));
            }
            setIsRunning(true);
        }
    }
    
    inline void stop()
    {
        setIsRunning(false);
    }
    
    /// @brief run a callable object on this thread
    /// @param newRunnable - any Callable object that will be executed on one of threads in the pool
    template<typename TCallable>
    inline void run(const TCallable& newCallable)
    {
        if (getIsRunning())
        {
            const std::lock_guard<decltype(mutex)> lock(mutex);
            queue.emplace(std::make_unique<RunnableWithObject<TCallable>>(newCallable));
            queueWait.notify_one();
        }
        else
        {
            throw std::runtime_error("Thread is not running");
        }
    }
    
    inline std::size_t getConcurency() const noexcept
    {
        return threadCount;
    }
    
    inline bool getIsRunning() const noexcept
    {
        return isRunning;
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
    std::size_t threadCount { 0 };
    std::vector<std::thread> threads;
    
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
        queueWait.notify_all();
    }
};

} // namespace gusc::Threads

#endif /* ThreadPool_hpp */
