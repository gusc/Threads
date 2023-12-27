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
        auto threadId = GetThreadId(threadHandle);
        // taken from https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2008/xcb2z8hs(v=vs.90)
        const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
        typedef struct tagTHREADNAME_INFO
        {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        } THREADNAME_INFO;
#pragma pack(pop)
        THREADNAME_INFO Info;
        Info.dwType = 0x1000;
        Info.szName = threadName.c_str();
        Info.dwThreadID = threadId;
        Info.dwFlags = 0;
        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(Info) / sizeof(ULONG_PTR), (ULONG_PTR*)&Info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

inline void setThisThreadName() noexcept
{
}

inline void setThreadPriority() noexcept
{
    auto threadHandle = thread.native_handle();
    if (priority == Priority::RealTime)
    {
        SetThreadPriority(threadHandle, THREAD_PRIORITY_TIME_CRITICAL);
    }
    else if (priority == Priority::High)
    {
        SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST);
    }
    else if (priority == Priority::Low)
    {
        SetThreadPriority(threadHandle, THREAD_PRIORITY_LOWEST);
    }
}

inline void setThisThreadPriority() noexcept
{
}
