//
//  ThreadWindows.hpp
//  Threads
//
//  Created by Gusts Kaksis on 27/12/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

inline void setThreadName() noexcept
{
    if (!threadName.empty())
    {
        auto threadHandle = thread.native_handle();
        pthread_setname_np(threadHandle, threadName.c_str());
    }
}

inline void setThisThreadName() noexcept
{
}

inline void setThreadPriority() noexcept
{
    auto threadHandle = thread.native_handle();
    auto policy = SCHED_FIFO;
    auto priority_min = sched_get_priority_min(policy);
    auto priority_max = sched_get_priority_max(policy);
    if (priority == Priority::RealTime)
    {
        sched_param param;
        param.sched_priority=priority_max;
        auto status = pthread_setschedparam(threadHandle, policy, &param);
        if (status != 0)
        {
            // TODO: Handle error
        }
    }
    else if (priority == Priority::High)
    {
        sched_param param;
        param.sched_priority=priority_max - 1;
        auto status = pthread_setschedparam(threadHandle, policy, &param);
        if (status != 0)
        {
            // TODO: Handle error
        }
    }
    else if (priority == Priority::Low)
    {
        sched_param param;
        param.sched_priority=priority_min + 1;
        auto status = pthread_setschedparam(threadHandle, policy, &param);
        if (status != 0)
        {
            // TODO: Handle error
        }
    }
}

inline void setThisThreadPriority() noexcept
{
}
