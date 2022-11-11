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

namespace gusc::Threads
{

/// @brief Class representing a new thread
class Thread
{
    class Task;
    
public:
    /// @brief Class representing a handle to delayed task and allows to check whether the task has executed and also cancel it prior it's moved to main task queue.
    class TaskHandle
    {
    public:
        TaskHandle() = default;
        TaskHandle(std::weak_ptr<Task> initTask)
            : task(initTask)
        {}
        virtual ~TaskHandle() = default;
        /// @brief cancel the delayed task
        inline void cancel() noexcept
        {
            if (auto t = task.lock())
            {
                t->cancel();
            }
        }
        /// @brief check if task was executed (i.e. it's moved to main task queue)
        inline bool isExecuted() const noexcept
        {
            return task.expired();
        }
    private:
        std::weak_ptr<Task> task;
    };
    
    template<typename TReturn>
    class TaskHandleWithFuture : public TaskHandle
    {
    public:
        TaskHandleWithFuture(std::weak_ptr<Task> initTask, std::future<TReturn> initFuture)
            : TaskHandle(initTask)
            , future(std::move(initFuture))
        {}
        
        TReturn getValue()
        {
            return future.get();
        }
    private:
        std::future<TReturn> future;
    };
    
    template<>
    class TaskHandleWithFuture<void> : public TaskHandle
    {
    public:
        TaskHandleWithFuture(std::weak_ptr<Task> initTask, std::future<void> initFuture)
            : TaskHandle(initTask)
            , future(std::move(initFuture))
        {}
        
        void getValue()
        {
            future.get();
        }
    private:
        std::future<void> future;
    };
    
    Thread() = default;
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;
    virtual ~Thread()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            setIsAcceptingTasks(false);
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
            setIsAcceptingTasks(true);
            setIsRunning(true);
            thread = std::make_unique<std::thread>(&Thread::runLoop, this);
        }
        else
        {
            throw std::runtime_error("Thread already started");
        }
    }
    
    /// @brief signal the thread to stop - this also stops receiving tasks
    /// @warning if a task is sent after calling this method an exception will be thrown
    virtual inline void stop()
    {
        if (thread)
        {
            std::lock_guard<std::mutex> lock(mutex);
            setIsRunning(false);
            setIsAcceptingTasks(false);
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
    
    /// @brief send a task that needs to be executed on this thread
    /// @param newTask - any callable object that will be executed on this thread
    template<typename TCallable>
    inline void send(const TCallable& newTask)
    {
        if (getIsAcceptingTasks())
        {
            std::lock_guard<std::mutex> lock(mutex);
            taskQueue.emplace(std::make_shared<TaskWithCallable<TCallable>>(newTask));
            queueWait.notify_one();
        }
        else
        {
            throw std::runtime_error("Thread is not accepting any tasks, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send a delayed task that needs to be executed on this thread
    /// @param newTask - any callable object that will be executed on this thread
    /// @return a TaskHandle object which allows you to cancel delayed task before it's timeout has expired
    /// @note once the task is moved from delayed queue to task queue it's TaskHandle object be expired and won't be cancellable any more
    template<typename TCallable>
    inline TaskHandle sendDelayed(const TCallable& newTask, const std::chrono::milliseconds& timeout)
    {
        if (getIsAcceptingTasks())
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto time = std::chrono::steady_clock::now() + timeout;
            auto task = std::make_shared<TaskWithCallable<TCallable>>(newTask);
            TaskHandle handle { task };
            delayedQueue.emplace(std::make_unique<DelayedTaskWrapper>(time, std::move(task)));
            queueWait.notify_one();
            return handle;
        }
        else
        {
            throw std::runtime_error("Thread is not accepting any tasks, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send an asynchronous task that returns value and needs to be executed on this thread (calling thread is not blocked)
    /// @note if sent from the same thread this method will call the callable immediatelly to prevent deadlocking
    /// @param newTask - any callable object that will be executed on this thread and it must return a value of type specified in TReturn (signature: TReturn(void))
    template<typename TReturn, typename TCallable>
    inline TaskHandleWithFuture<TReturn> sendAsync(const TCallable& newTask)
    {
        if (getIsAcceptingTasks())
        {
            std::promise<TReturn> promise;
            auto future = promise.get_future();
            auto task = std::make_shared<TaskWithPromise<TReturn, TCallable>>(newTask, std::move(promise));
            TaskHandleWithFuture<TReturn> handle(task, std::move(future));
            if (getIsSameThread())
            {
                // If we are on the same thread excute task immediatelly to prent a deadlock
                task->execute();
            }
            else
            {
                std::lock_guard<std::mutex> lock(mutex);
                taskQueue.emplace(task);
                queueWait.notify_one();
            }
            return handle;
        }
        else
        {
            throw std::runtime_error("Thread is not accepting any tasks, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send a synchronous task that returns value and needs to be executed on this thread (calling thread is blocked until task returns)
    /// @note to prevent deadlocking this method throws exception if called before thread has started
    /// @param newTask - any callable object that will be executed on this thread and it must return a value of type specified in TReturn (signature: TReturn(void))
    template<typename TReturn, typename TCallable>
    inline TReturn sendSync(const TCallable& newTask)
    {
        if (!getIsRunning())
        {
            throw std::runtime_error("Can not place a blocking task if the thread is not started");
        }
        auto handle = sendAsync<TReturn>(newTask);
        return handle.getValue();
    }
    
    /// @brief send a task that needs to be executed on this thread and wait for it's completion
    /// @param newTask - any callable object that will be executed on this thread
    template<typename TCallable>
    inline void sendWait(const TCallable& newTask)
    {
        sendSync<void>(newTask);
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
            std::shared_ptr<Task> next;
            {
                std::unique_lock<std::mutex> lock(mutex);
                // Move delayed tasks to main queue
                enqueueDelayedTasks();
                // Get the next task from the main queue
                if (!taskQueue.empty())
                {
                    next = std::move(taskQueue.front());
                    taskQueue.pop();
                }
                else if (!delayedQueue.empty())
                {
                    // There are no tasks to process, but delayedQueue had some tasks, we can wait till delay expires
                    queueWait.wait_until(lock, (*delayedQueue.begin())->getTime());
                }
                else
                {
                    // We wait for a new task to be pushed on any of the queues
                    queueWait.wait(lock);
                }
            }
            if (next)
            {
                next->execute();
            }
        }
        runLeftovers();
    }
    
    inline void enqueueDelayedTasks()
    {
        const auto timeNow = std::chrono::steady_clock::now();
        for (auto it = delayedQueue.begin(); it != delayedQueue.end();)
        {
            if ((*it)->getTime() < timeNow)
            {
                auto ptr = (*it)->moveTask();
                if (ptr)
                {
                    taskQueue.emplace(std::move(ptr));
                }
                it = delayedQueue.erase(it);
            }
            else
            {
                break;
            }
        }
    }
    
    inline void runLeftovers()
    {
        // Process any leftover tasks
        std::lock_guard<std::mutex> lock(mutex);
        while (taskQueue.size())
        {
            taskQueue.front()->execute();
            taskQueue.pop();
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
    
    inline bool getIsAcceptingTasks() const noexcept
    {
        return isAcceptingTasks;
    }

    inline void setIsAcceptingTasks(bool newIsAcceptingTasks) noexcept
    {
        isAcceptingTasks = newIsAcceptingTasks;
    }
    
    inline bool getIsSameThread() const noexcept
    {
        return getId() == std::this_thread::get_id();
    }

private:
    
    /// @brief base class for thread task
    class Task
    {
    public:
        virtual ~Task() = default;
        virtual void execute() {}
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
    
    /// @brief templated task to wrap a callable object
    template<typename TCallable>
    class TaskWithCallable : public Task
    {
    public:
        TaskWithCallable(const TCallable& initCallableObject)
            : callableObject(initCallableObject)
        {}
        inline void execute() override
        {
            try
            {
                if (!getIsCancelled())
                {
                    callableObject();
                }
            }
            catch(...)
            {
                // We can't do nothing as nobody is listening, but we don't want the thread to explode
            }
        }
    private:
        TCallable callableObject;
    };
    
    /// @brief templated task to wrap a callable object which accepts promise object that can be used to signal finish of the callable (useful for subsequent async calls)
    template<typename TReturn, typename TCallable>
    class TaskWithPromise : public Task
    {
    public:
        TaskWithPromise(const TCallable& initCallableObject, std::promise<TReturn> initWaitablePromise)
            : callableObject(initCallableObject)
            , waitablePromise(std::move(initWaitablePromise))
        {}
        inline void execute() override
        {
            if (!getIsCancelled())
            {
                actualCall<TReturn>();
            }
            else
            {
                try
                {
                    // As this task was cancelled we report back a broken promise exception
                    waitablePromise.set_exception(std::future_error(std::future_errc::broken_promise));
                }
                catch(...)
                {
                    // We can't do nothing as nobody is listening, but we don't want the thread to explode
                }
            }
        }
    private:
        TCallable callableObject;
        std::promise<TReturn> waitablePromise;
        
        template<typename TRet>
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
            try
            {
                callableObject();
                waitablePromise.set_value();
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
    };
    
    /// @brief templated task to wrap a callable object
    class DelayedTaskWrapper
    {
    public:
        DelayedTaskWrapper(std::chrono::time_point<std::chrono::steady_clock> initTime,
                           std::shared_ptr<Task> initTask)
            : time(initTime)
            , task(std::move(initTask))
        {}
        inline bool operator<(const DelayedTaskWrapper& other)
        {
            return time < other.getTime();
        }
        inline std::shared_ptr<Task> moveTask()
        {
            return std::move(task);
        }
        inline std::chrono::time_point<std::chrono::steady_clock> getTime() const noexcept
        {
            return time;
        }
    private:
        const std::chrono::time_point<std::chrono::steady_clock> time {};
        std::shared_ptr<Task> task;
    };

    std::atomic<bool> isRunning { false };
    std::atomic<bool> isAcceptingTasks { true };
    std::queue<std::shared_ptr<Task>> taskQueue;
    std::multiset<std::unique_ptr<DelayedTaskWrapper>> delayedQueue;
    std::unique_ptr<std::thread> thread;
    std::condition_variable queueWait;
    std::mutex mutex;
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
        setIsAcceptingTasks(true);
        setIsRunning(true);
        runLoop();
    }

    /// @brief signal the thread to stop - this also stops receiving tasks
    /// @warning if a task is sent after calling this method an exception will be thrown
    inline void stop() override
    {
        setIsRunning(false);
        setIsAcceptingTasks(false);
    }
};
    
}

#endif /* Thread_hpp */
