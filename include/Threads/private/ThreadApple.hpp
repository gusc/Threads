//
//  ThreadApple.hpp
//  Threads
//
//  Created by Gusts Kaksis on 27/12/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

inline void setThreadName() noexcept
{
}

inline void setThisThreadName() noexcept
{
    if (!threadName.empty())
    {
        pthread_setname_np(threadName.c_str());
    }
}

inline void setThreadPriority() noexcept
{
}

inline void setThisThreadPriority() noexcept
{
    if (priority == Priority::RealTime)
    {
        // Copy-pasta from: https://developer.apple.com/library/archive/technotes/tn2169/_index.html#//apple_ref/doc/uid/DTS40013172-CH1-TNTAG6000
        mach_timebase_info_data_t timebaseInfo;
        mach_timebase_info(&timebaseInfo);

        const auto timeframe = 0.01; // 10ms
        const auto time = timeframe * NSEC_PER_SEC;
        const auto freq = static_cast<float>(timebaseInfo.denom) / timebaseInfo.numer;
        const auto deadline = static_cast<uint32_t>(time * freq);

        thread_time_constraint_policy_data_t policy;
        policy.period      = 0;
        policy.computation = deadline;
        policy.constraint  = deadline * 2;
        policy.preemptible = false;

        int kr = thread_policy_set(pthread_mach_thread_np(pthread_self()),
                                   THREAD_TIME_CONSTRAINT_POLICY,
                                   (thread_policy_t)&policy,
                                   THREAD_TIME_CONSTRAINT_POLICY_COUNT);
        if (kr != KERN_SUCCESS)
        {
            // TODO: Handle error
        }
    }
    else if (priority == Priority::High)
    {
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 32);
    }
    else if (priority == Priority::Low)
    {
        pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
    }
}
