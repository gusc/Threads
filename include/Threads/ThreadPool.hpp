//
//  ThreadPool.hpp
//  Threads
//
//  Created by Gusts Kaksis on 13/11/2022.
//  Copyright © 2022 Gusts Kaksis. All rights reserved.
//

#ifndef GUSC_THREADPOOL_HPP
#define GUSC_THREADPOOL_HPP

#include "Thread.hpp"
#include <vector>
#include <memory>

namespace gusc::Threads
{

///@brief Class representing a thread pool - multiple threads running same callable object
class ThreadPool final
{
public:
    ThreadPool() = default;
    // Thread("name", function, args...)
    template <class TFn, class ...TArgs>
    ThreadPool(const std::string& initThreadPoolName, std::size_t initThreadPoolSize, TFn&& fn, TArgs&&... args)
        : threadPoolName(initThreadPoolName)
        , threadProcedure(std::forward<TFn>(fn), std::forward<TArgs>(args)...)
    {
        for (std::size_t i = 0; i < initThreadPoolSize; ++i)
        {
            threads.emplace_back(std::make_unique<Thread>(threadPoolName + "[" + std::to_string(i) + "]", threadProcedure));
        }
    }
    // Thread(function, args...)
    template <class TFn, class ...TArgs,
        class = typename std::enable_if<
            !IsSameType<TFn, std::string>::value &&
            !IsSameType<TFn, char*>::value
        >::type
    >
    explicit ThreadPool(std::size_t initThreadPoolSize, TFn&& fn, TArgs&&... args)
        : ThreadPool("gust::Threads::ThreadPool", initThreadPoolSize, std::forward<TFn>(fn), std::forward<TArgs>(args)...)
    {}
    
    inline void resize(std::size_t newSize)
    {
        if (isStarted)
        {
            throw std::runtime_error("ThreadPool can not be resized while running");
        }
        if (newSize > threads.size())
        {
            // Append new threads
            const auto initSize = threads.size();
            for (std::size_t i = initSize; i < newSize; ++i)
            {
                threads.emplace_back(std::make_unique<Thread>(threadPoolName + "[" + std::to_string(i) + "]", threadProcedure));
            }
        }
        else if (newSize < threads.size())
        {
            // Remove threads from the end
            threads.erase(threads.end() - (threads.size() - newSize), threads.end());
        }
    }
    
    inline void start()
    {
        if (isStarted)
        {
            throw std::runtime_error("ThreadPool has already started");
        }
        isStarted = true;
        for (auto& t : threads)
        {
            try
            {
                t->start();
            }
            catch (...)
            {}
        }
    }
    
    inline void stop()
    {
        if (!isStarted)
        {
            throw std::runtime_error("ThreadPool has not been started");
        }
        for (auto& t : threads)
        {
            try
            {
                t->stop();
            }
            catch (...)
            {}
        }
        isStarted = false;
    }
    
    inline std::size_t getSize() const noexcept
    {
        return threads.size();
    }
    
    inline bool getIsStarted() const noexcept
    {
        return isStarted;
    }
    
    inline bool getIsThreadIdInPool(std::thread::id threadId) const noexcept
    {
        for (auto& t : threads)
        {
            if (t->getId() == threadId)
            {
                return true;
            }
        }
        return false;
    }

private:
    #include "private/ThreadStructures.hpp"
    
    std::vector<std::unique_ptr<Thread>> threads;
    std::atomic_bool isStarted { false };
    std::string threadPoolName { "gust::Threads::ThreadPool" };
    ThreadProcedure threadProcedure;
};

} // namespace gusc::Threads

#endif /* GUSC_THREADPOOL_HPP */
