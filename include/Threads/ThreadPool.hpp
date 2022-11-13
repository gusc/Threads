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
        if (getIsRunning())
        {
            stop();
        }
    }
    
    inline void start()
    {
        if (!getIsRunning())
        {
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
        if (getIsRunning())
        {
            setIsRunning(false);
            threads.clear();
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
    std::size_t threadCount { 0 };
    std::vector<std::thread> threads;
    std::atomic_bool isRunning { false };
    
    inline void runLoop()
    {
        while (isRunning)
        {
            std::this_thread::sleep_for(1s);
        }
    }
    
    inline void setIsRunning(bool newIsRunning) noexcept
    {
        isRunning = newIsRunning;
    }
};

} // namespace gusc::Threads

#endif /* ThreadPool_hpp */
