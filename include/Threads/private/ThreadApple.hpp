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

