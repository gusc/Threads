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
    
    /// @brief start the thread and it's run-loop
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
    
    /// @brief signal the thread to stop - this also stops receiving messages
    /// @warning if a message is sent after calling this method an exception will be thrown
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
    
    /// @brief join the thread and wait unti it's finished
    void join()
    {
        if (thread && thread->joinable())
        {
            thread->join();
        }
    }
    
    /// @brief send a message that needs to be executed on this thread
    /// @param newMessage - any callable object that will be executed on this thread
    template<typename TCallable>
    void send(const TCallable& newMessage)
    {
        if (getIsAcceptingMessages())
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(std::make_unique<CallableMessage<TCallable>>(newMessage));
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
    
    /// @brief start the thread and it's run-loop
    /// @warning calling this method will efectivelly block current thread
    void start() override
    {
        runLoop();
    }

    /// @brief signal the thread to stop - this also stops receiving messages
    /// @warning if a message is sent after calling this method an exception will be thrown
    void stop() override
    {
        setIsAcceptingMessages(false);
        setIsRunning(false);
    }
};
    
}

#endif /* Thread_hpp */
