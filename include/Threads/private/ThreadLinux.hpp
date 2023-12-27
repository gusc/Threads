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

