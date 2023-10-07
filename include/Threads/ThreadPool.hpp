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

///@brief Class representing a thread pool
class ThreadPool
{
public:
    ThreadPool(std::size_t initThreadCount, const std::function<void(const Thread::StopToken&)>& initThreadProcedure)
    {
        for (std::size_t i = 0; i < initThreadCount; ++i)
        {
            threads.emplace_back(std::make_unique<Thread>(initThreadProcedure));
        }
    }
    ThreadPool(std::size_t initThreadCount, const std::function<void(void)>& initThreadProcedure)
    {
        for (std::size_t i = 0; i < initThreadCount; ++i)
        {
            threads.emplace_back(std::make_unique<Thread>([initThreadProcedure](const Thread::StopToken&){
                initThreadProcedure();
            }));
        }
    }
    
    inline void start()
    {
        for (auto& t : threads)
        {
            t->start();
        }
    }
    
    inline void stop()
    {
        for (auto& t : threads)
        {
            t->stop();
        }
    }
    
    inline std::size_t getConcurency() const noexcept
    {
        return threads.size();
    }

private:
    std::vector<std::unique_ptr<Thread>> threads;
};

} // namespace gusc::Threads

#endif /* GUSC_THREADPOOL_HPP */
