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
#include <chrono>
#include <queue>
#include <mutex>
#include <utility>

namespace
{
constexpr const std::size_t MaxSpinCycles { 1000 };
}

namespace gusc::Threads
{

/// @brief Class representing a new thread
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
        setIsAcceptingMessages(false);
        setIsRunning(false);
        join();
    }
    
    virtual void start()
    {
        if (!getIsRunning())
        {
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
            setIsAcceptingMessages(false);
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
    
    template<typename TCallable>
    void send(const TCallable& newMessage)
    {
        if (getIsAcceptingMessages())
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(new CallableMessage<TCallable>(newMessage));
        }
        else
        {
            throw std::runtime_error("Thread is not excepting any messages, the thread has been signaled for stopping");
        }
    }
    
    template<typename TCallable>
    void send(TCallable&& newMessage)
    {
        if (getIsAcceptingMessages())
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(new CallableMessage<TCallable>(std::forward<TCallable>(newMessage)));
        }
        else
        {
            throw std::runtime_error("Thread is not excepting any messages, the thread has been signaled for stopping");
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
                missCounter = 0;
                next->call();
            }
            else
            {
                if (missCounter < MaxSpinCycles)
                {
                    ++missCounter;
                    std::this_thread::yield();
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(1));
                }
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
    
    inline bool getIsAcceptingMessages() const noexcept
    {
        return isAcceptingMessages;
    }

    inline void setIsAcceptingMessages(bool newIsAcceptingMessages) noexcept
    {
        isAcceptingMessages = newIsAcceptingMessages;
    }

private:
    
    /// @brief base class for thread message
    class Message
    {
    public:
        virtual void call() {}
    };
    
    /// @brief templated message to wrap a callable object
    template<typename TCallable>
    class CallableMessage : public Message
    {
    public:
        CallableMessage(const TCallable& initCallableObject)
            : callableObject(initCallableObject)
        {}
        CallableMessage(TCallable&& initCallableObject)
            : callableObject(std::forward<TCallable>(initCallableObject))
        {}
        void call() override
        {
            callableObject();
        }
    private:
        TCallable callableObject;
    };
    
    std::size_t missCounter { 0 };
    std::atomic<bool> isRunning { false };
    std::atomic<bool> isAcceptingMessages { true };
    std::queue<std::unique_ptr<Message>> messageQueue;
    std::unique_ptr<std::thread> thread;
    std::mutex messageMutex;
};

/// @brief Class representing a currently executing thread
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
        runLoop();
    }

    void stop() override
    {
        setIsAcceptingMessages(false);
        setIsRunning(false);
    }
};
    
}

#endif /* Thread_hpp */
