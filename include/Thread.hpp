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
    Thread() = default;
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;
    ~Thread()
    {
        setIsRunning(false);
        join();
    }
    
    virtual void start()
    {
        if (!getIsRunning())
        {
            if (thread && thread->joinable())
            {
                thread->join();
            }
            setIsRunning(true);
            thread = std::make_unique<std::thread>(&Thread::runLoop, this);
        }
        else
        {
            throw std::runtime_error("Thread already started");
        }
    }
    
    virtual void stop()
    {
        if (thread)
        {
            setIsRunning(false);
        }
        else
        {
            throw std::runtime_error("Thread has not been started");
        }
    }
    
    void join()
    {
        if (thread && thread->joinable())
        {
            thread->join();
        }
    }
    
    template<typename TMsg>
    void send(const TMsg& newMessage)
    {
        if (getIsRunning())
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(new TMessage<TMsg>(newMessage));
        }
        else
        {
            throw std::runtime_error("Thread is not excepting any messages, you have to start it first");
        }
    }
    
    template<typename TMsg>
    void send(TMsg&& newMessage)
    {
        std::lock_guard<std::mutex> lock(messageMutex);
        messageQueue.emplace(new TMessage<TMsg>(std::forward<TMsg>(newMessage)));
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
        while (getIsRunning())
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
        runLeftovers();
    }
    
    void runLeftovers()
    {
        // Process any leftover messages
        std::lock_guard<std::mutex> lock(messageMutex);
        while (messageQueue.size())
        {
            messageQueue.front()->call();
            messageQueue.pop();
        }
    }
    
    inline std::thread::id getId() const noexcept
    {
        if (thread)
        {
            return thread->get_id();
        }
        else
        {
            // We haven't started a thread yet, so we're the same thread
            return std::this_thread::get_id();
        }
    }
    
    inline bool getIsRunning() const noexcept
    {
        return isRunning;
    }

    inline void setIsRunning(bool newIsRunning) noexcept
    {
        isRunning = newIsRunning;
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
    std::atomic<bool> isRunning { false };
    std::queue<std::unique_ptr<Message>> messageQueue;
    std::mutex messageMutex;
};

class ThisThread : public Thread
{
public:
    ThisThread()
    {
        // ThisThread is already running
        setIsRunning(true);
    }
    
    void start() override
    {
        throw std::runtime_error("ThisThread is already started");
    }

    void stop() override
    {
        setIsRunning(false);
    }
    
    void run()
    {
        runLoop();
    }
};
    
}

#endif /* Thread_hpp */
