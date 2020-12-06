//
//  Thread.hpp
//  Threads
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#ifndef Thread_hpp
#define Thread_hpp

#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <utility>

namespace gusc::Threads
{

class Thread
{
public:
    Thread()
        : thread(std::make_unique<std::thread>(&Thread::runLoop, this))
    {}
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;
    ~Thread()
    {
        quit();
        join();
    }
    
    void join()
    {
        if (thread && thread->joinable())
        {
            thread->join();
        }
    }
    
    void quit()
    {
        continueRunning = false;
    }
    
    template<typename TMsg>
    void send(const TMsg& newMessage)
    {
        if (acceptMessages)
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(new TMessage<TMsg>(newMessage));
        }
    }
    
    template<typename TMsg>
    void send(TMsg&& newMessage)
    {
        if (acceptMessages)
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(new TMessage<TMsg>(std::forward<TMsg>(newMessage)));
        }
    }
    
    inline bool operator==(const Thread& other) const noexcept
    {
        return getId() == other.getId();
    }
    inline bool operator!=(const Thread& other) const noexcept
    {
        return !(operator==(other));
    }
    
    inline bool operator==(const std::thread::id& other) const noexcept
    {
        return getId() == other;
    }
    inline bool operator!=(const std::thread::id& other) const noexcept
    {
        return !(operator==(other));
    }
    
protected:
    void runLoop()
    {
        while (continueRunning)
        {
            std::unique_ptr<Message> next;
            {
                std::lock_guard<std::mutex> lock(messageMutex);
                if (messageQueue.size())
                {
                    next = std::move(messageQueue.front());
                    messageQueue.pop();
                }
            }
            if (next)
            {
                next->call();
            }
            else
            {
                std::this_thread::yield();
            }
        }
        acceptMessages = false;
        {
            // Process any leftover messages
            std::lock_guard<std::mutex> lock(messageMutex);
            while (messageQueue.size())
            {
                messageQueue.front()->call();
                messageQueue.pop();
            }
        }
    }
    
    virtual inline std::thread::id getId() const noexcept
    {
        return thread->get_id();
    }

private:
    class Message
    {
    public:
        virtual void call() {}
    };
    template<typename TMsg>
    class TMessage : public Message
    {
    public:
        TMessage(const TMsg& initMessageObject)
            : messageObject(initMessageObject)
        {}
        TMessage(TMsg&& initMessageObject)
            : messageObject(std::forward<TMsg>(initMessageObject))
        {}
        void call() override
        {
            messageObject();
        }
    private:
        TMsg messageObject;
    };
    
    std::unique_ptr<std::thread> thread;
    std::atomic<bool> acceptMessages { true };
    std::atomic<bool> continueRunning { true };
    std::queue<std::unique_ptr<Message>> messageQueue;
    std::mutex messageMutex;
};

class MainThread : public Thread
{
public:
    MainThread()
        : threadId(std::this_thread::get_id())
    {
    }
    
    void run()
    {
        runLoop();
    }
    
protected:
    inline std::thread::id getId() const noexcept override
    {
        return threadId;
    }
private:
    std::thread::id threadId;
};
    
}

#endif /* Thread_hpp */
