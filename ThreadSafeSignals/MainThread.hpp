//
//  MainThread.hpp
//  ThreadSafeSignals
//
//  Created by Gusts Kaksis on 22/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#ifndef MainThread_hpp
#define MainThread_hpp

#include <thread>

#include "Signal.hpp"

namespace gusc::Threads
{

class MainThread : public Thread
{
public:
    MainThread()
        : Thread(Thread::UninitializedTag{})
        , threadId(std::this_thread::get_id())
    {
        // onQuit(Thread*, void)
        sigQuit.connect(this, &MainThread::onQuit);
    }
    
    void run()
    {
        runLoop();
    }
    
    Signal<void> sigQuit;
    
private:
    std::thread::id threadId;
    
    void onQuit()
    {
        quit();
    }
};
    
}

#endif /* MainThread_hpp */
