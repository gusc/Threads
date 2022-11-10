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
#include <set>
#include <mutex>
#include <utility>
#include <future>
#include <iostream>

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
    
    class Cancellable
    {
    public:
        inline void cancel() noexcept
        {
            isCancelled = true;
        }
    protected:
        inline bool getIsCancelled() const noexcept
        {
            return isCancelled;
        }
    private:
        std::atomic_bool isCancelled { false };
    };
    
    Thread() = default;
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;
    virtual ~Thread()
    {
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            setIsAcceptingMessages(false);
            setIsRunning(false);
            queueWait.notify_one();
        }
        join();
    }
    
    /// @brief start the thread and it's run-loop
    virtual inline void start()
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
    virtual inline void stop()
    {
        if (thread)
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            setIsAcceptingMessages(false);
            setIsRunning(false);
            queueWait.notify_one();
        }
        else
        {
            throw std::runtime_error("Thread has not been started");
        }
    }
    
    /// @brief join the thread and wait unti it's finished
    inline void join()
    {
        if (thread && thread->joinable())
        {
            thread->join();
        }
    }
    
    /// @brief send a message that needs to be executed on this thread
    /// @param newMessage - any callable object that will be executed on this thread
    template<typename TCallable>
    inline void send(const TCallable& newMessage)
    {
        if (getIsAcceptingMessages())
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            messageQueue.emplace(std::make_unique<CallableMessage<TCallable>>(newMessage));
            queueWait.notify_one();
        }
        else
        {
            throw std::runtime_error("Thread is not excepting any messages, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send a delayed message that needs to be executed on this thread
    /// @param newMessage - any callable object that will be executed on this thread
    /// @return a weak pointer to Cancellable object which allows you to cancel delayed task before it's timeout has expired
    template<typename TCallable>
    inline std::weak_ptr<Cancellable> sendDelayed(const TCallable& newMessage, const std::chrono::milliseconds& timeout)
    {
        if (getIsAcceptingMessages())
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            auto time = std::chrono::steady_clock::now() + timeout;
            auto ref = delayedQueue.emplace(std::make_shared<DelayedMessageWrapper>(time, std::make_unique<CallableMessage<TCallable>>(newMessage)));
            queueWait.notify_one();
            return *ref;
        }
        else
        {
            throw std::runtime_error("Thread is not excepting any messages, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send an asynchronous message that returns value and needs to be executed on this thread (calling thread is not blocked)
    /// @note if sent from the same thread this method will call the callable immediatelly to prevent deadlocking
    /// @param newMessage - any callable object that will be executed on this thread and it must return a value of type specified in TReturn (signature: TReturn(void))
    template<typename TReturn, typename TCallable>
    inline std::future<TReturn> sendAsync(const TCallable& newMessage)
    {
        if (getIsAcceptingMessages())
        {
            std::promise<TReturn> promise;
            std::future<TReturn> future = promise.get_future();
            if (getIsSameThread())
            {
                // If we are on the same thread excute message immediatelly to prent a deadlock
                auto message = std::make_unique<CallableMessageWithPromise<TReturn, TCallable>>(newMessage, std::move(promise));
                message->call();
            }
            else
            {
                std::lock_guard<std::mutex> lock(messageMutex);
                messageQueue.emplace(std::make_unique<CallableMessageWithPromise<TReturn, TCallable>>(newMessage, std::move(promise)));
                queueWait.notify_one();
            }
            return future;
        }
        else
        {
            throw std::runtime_error("Thread is not excepting any messages, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send a synchronous message that returns value and needs to be executed on this thread (calling thread is blocked until message returns)
    /// @note to prevent deadlocking this method throws exception if called before thread has started
    /// @param newMessage - any callable object that will be executed on this thread and it must return a value of type specified in TReturn (signature: TReturn(void))
    template<typename TReturn, typename TCallable>
    inline TReturn sendSync(const TCallable& newMessage)
    {
        if (!getIsRunning())
        {
            throw std::runtime_error("Can not place a blocking message if the thread is not started");
        }
        auto future = sendAsync<TReturn>(newMessage);
        return future.get();
    }
    
    /// @brief send a message that needs to be executed on this thread and wait for it's completion
    /// @param newMessage - any callable object that will be executed on this thread
    template<typename TCallable>
    inline void sendWait(const TCallable& newMessage)
    {
        sendSync<void>(newMessage);
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
    inline void runLoop()
    {
        while (getIsRunning())
        {
            std::unique_ptr<Message> next;
            {
                const auto timeNow = std::chrono::steady_clock::now();
                std::unique_lock<std::mutex> lock(messageMutex);
                // Wait for any messages to arrive
                if (messageQueue.empty() && delayedQueue.empty())
                {
                    // We wait for a new message to be pushed on any of the queues
                    queueWait.wait(lock);
                }
                // Move delayed messages to main queue
                if (!delayedQueue.empty())
                {
                    auto hasNew { false };
                    for (auto it = delayedQueue.begin(); it != delayedQueue.end();)
                    {
                        if ((*it)->getTime() < timeNow)
                        {
                            auto ptr = (*it)->getMessage();
                            if (ptr)
                            {
                                messageQueue.emplace(std::move(ptr));
                            }
                            it = delayedQueue.erase(it);
                            hasNew = true;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (!hasNew)
                    {
                        // If there are queued items but none were added to the queue wait till next queued item
                        auto nextTime = std::lower_bound(delayedQueue.begin(), delayedQueue.end(), timeNow,
                                                         [](const std::shared_ptr<DelayedMessageWrapper>& a,
                                                            const std::chrono::time_point<std::chrono::steady_clock>& b){
                            return a->getTime() < b;
                        });
                        if (nextTime != delayedQueue.end())
                        {
                            queueWait.wait_until(lock, (*nextTime)->getTime());
                        }
                    }
                }
                // Get the next message from the main queue
                if (!messageQueue.empty())
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
        }
        runLeftovers();
    }
    
    inline void runLeftovers()
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
    
    inline bool getIsSameThread() const noexcept
    {
        return getId() == std::this_thread::get_id();
    }

private:
    
    /// @brief base class for thread message
    class Message
    {
    public:
        virtual ~Message() = default;
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
        inline void call() override
        {
            try
            {
                callableObject();
            }
            catch(...)
            {
                // We can't do nothing as nobody is listening, but we don't want the thread to explode
            }
        }
    private:
        TCallable callableObject;
    };
    
    /// @brief templated message to wrap a callable object
    class DelayedMessageWrapper : public Cancellable
    {
    public:
        DelayedMessageWrapper(std::chrono::time_point<std::chrono::steady_clock> initTime,
                              std::unique_ptr<Message> initMessage)
            : time(initTime)
            , message(std::move(initMessage))
        {}
        bool operator<(const DelayedMessageWrapper& other)
        {
            return time < other.getTime();
        }
        inline std::unique_ptr<Message> getMessage()
        {
            if (getIsCancelled())
            {
                return nullptr;
            }
            return std::move(message);
        }
        inline std::chrono::time_point<std::chrono::steady_clock> getTime() const noexcept
        {
            return time;
        }
    private:
        std::chrono::time_point<std::chrono::steady_clock> time {};
        std::unique_ptr<Message> message;
    };
    
    /// @brief templated message to wrap a callable object which accepts promise object that can be used to signal finish of the callable (useful for subsequent async calls)
    template<typename TReturn, typename TCallable>
    class CallableMessageWithPromise : public Message
    {
    public:
        CallableMessageWithPromise(const TCallable& initCallableObject, std::promise<TReturn> initWaitablePromise)
            : callableObject(initCallableObject)
            , waitablePromise(std::move(initWaitablePromise))
        {}
        inline void call() override
        {
            actualCall<TReturn>();
        }
    private:
        TCallable callableObject;
        std::promise<TReturn> waitablePromise;
        
        template<typename TR>
        void actualCall()
        {
            try
            {
                waitablePromise.set_value(callableObject());
            }
            catch(...)
            {
                try
                {
                    waitablePromise.set_exception(std::current_exception());
                }
                catch(...)
                {
                    // We can't do nothing as nobody is listening, but we don't want the thread to explode
                }
            }
        }
        
        template<>
        inline void actualCall<void>()
        {
            callableObject();
            waitablePromise.set_value();
        }
        
    };
    
    std::size_t missCounter { 0 };
    std::atomic<bool> isRunning { false };
    std::atomic<bool> isAcceptingMessages { true };
    std::queue<std::unique_ptr<Message>> messageQueue;
    std::multiset<std::shared_ptr<DelayedMessageWrapper>> delayedQueue;
    std::unique_ptr<std::thread> thread;
    std::condition_variable queueWait;
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
    inline void start() override
    {
        runLoop();
    }

    /// @brief signal the thread to stop - this also stops receiving messages
    /// @warning if a message is sent after calling this method an exception will be thrown
    inline void stop() override
    {
        setIsAcceptingMessages(false);
        setIsRunning(false);
    }
};
    
}

#endif /* Thread_hpp */
