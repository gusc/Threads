//
//  SerialTaskQueue.hpp
//  Threads
//
//  Created by Gusts Kaksis on 11/11/2022.
//  Copyright © 2022 Gusts Kaksis. All rights reserved.
//

#ifndef SerialTaskQueue_hpp
#define SerialTaskQueue_hpp

#include "Thread.hpp"
#include "Utilities.hpp"
#include <set>
#include <mutex>
#include <utility>
#include <future>
#include <queue>

namespace gusc::Threads
{

/// @brief Class representing a base task queue
class TaskQueue
{
protected:
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
    
    TaskQueue() = default;
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    TaskQueue(TaskQueue&&) = delete;
    TaskQueue& operator=(TaskQueue&&) = delete;
    ~TaskQueue() = default;

    /// @brief send a task that needs to be executed on this thread
    /// @param newTask - any callable object that will be executed on this thread
    template<typename TCallable>
    inline void send(const TCallable& newTask)
    {
        if (getAcceptsTasks())
        {
            std::lock_guard<std::mutex> lock(mutex);
            taskQueue.emplace(std::make_shared<TaskWithCallable<TCallable>>(newTask));
            notifyQueueChange();
        }
        else
        {
            throw std::runtime_error("Task queue is not accepting any tasks, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send a delayed task that needs to be executed on this thread
    /// @param newTask - any callable object that will be executed on this thread
    /// @return a TaskHandle object which allows you to cancel delayed task before it's timeout has expired
    /// @note once the task is moved from delayed queue to task queue it's TaskHandle object be expired and won't be cancellable any more
    template<typename TCallable>
    inline TaskHandle sendDelayed(const TCallable& newTask, const std::chrono::milliseconds& timeout)
    {
        if (getAcceptsTasks())
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto time = std::chrono::steady_clock::now() + timeout;
            auto task = std::make_shared<TaskWithCallable<TCallable>>(newTask);
            TaskHandle handle { task };
            delayedQueue.emplace(std::make_unique<DelayedTaskWrapper>(time, std::move(task)));
            notifyQueueChange();
            return handle;
        }
        else
        {
            throw std::runtime_error("Task queue is not accepting any tasks, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send an asynchronous task that returns value and needs to be executed on this thread (calling thread is not blocked)
    /// @note if sent from the same thread this method will call the callable immediatelly to prevent deadlocking
    /// @param newTask - any callable object that will be executed on this thread and it must return a value of type specified in TReturn (signature: TReturn(void))
    template<typename TReturn, typename TCallable>
    inline TaskHandleWithFuture<TReturn> sendAsync(const TCallable& newTask)
    {
        if (getAcceptsTasks())
        {
            std::promise<TReturn> promise;
            auto future = promise.get_future();
            auto task = std::make_shared<TaskWithPromise<TReturn, TCallable>>(newTask, std::move(promise));
            TaskHandleWithFuture<TReturn> handle(task, std::move(future));
            if (getIsSameThread())
            {
                // If we are on the same thread excute task immediatelly to prevent a deadlock
                task->execute();
            }
            else
            {
                std::lock_guard<std::mutex> lock(mutex);
                taskQueue.emplace(task);
                notifyQueueChange();
            }
            return handle;
        }
        else
        {
            throw std::runtime_error("Task queue is not accepting any tasks, the thread has been signaled for stopping");
        }
    }
    
    /// @brief send a synchronous task that returns value and needs to be executed on this thread (calling thread is blocked until task returns)
    /// @note to prevent deadlocking this method throws exception if called before thread has started
    /// @param newTask - any callable object that will be executed on this thread and it must return a value of type specified in TReturn (signature: TReturn(void))
    template<typename TReturn, typename TCallable>
    inline TReturn sendSync(const TCallable& newTask)
    {
        if (!getAcceptsTasks())
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
    
    /// @brief Check if we are on caller is on the same thread as the task queue
    inline bool getIsSameThread() const noexcept
    {
        return threadId == std::this_thread::get_id();
    }
    
    /// @brief Get if task queue accepts new tasks
    inline bool getAcceptsTasks() const noexcept
    {
        return acceptsTasks;
    }
    
protected:
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
                    waitablePromise.set_exception(std::make_exception_ptr(std::future_error(std::future_errc::broken_promise)));
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
    
    inline std::chrono::time_point<std::chrono::steady_clock> enqueueDelayedTasks(std::chrono::time_point<std::chrono::steady_clock> timeNow)
    {
        const std::lock_guard<decltype(mutex)> lock(mutex);
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
        if (!delayedQueue.empty())
        {
            return (*delayedQueue.begin())->getTime();
        }
        return timeNow;
    }
    
    inline void setThreadId(std::thread::id newThreadId)
    {
        threadId = newThreadId;
    }
    
    inline void setAcceptsTasks(bool newAcceptsTasks)
    {
        acceptsTasks = newAcceptsTasks;
    }
    
    inline virtual void notifyQueueChange()
    {}
    
    inline LockedReference<std::queue<std::shared_ptr<Task>>, std::mutex> acquireTaskQueue()
    {
        return { taskQueue, mutex };
    }
    
private:
    std::mutex mutex;
    std::thread::id threadId { std::this_thread::get_id() };
    std::atomic_bool acceptsTasks { true };
    std::queue<std::shared_ptr<Task>> taskQueue;
    std::multiset<std::unique_ptr<DelayedTaskWrapper>> delayedQueue;
};

/// @brief a class representing a task queue that's running serially on a single thread
class SerialTaskQueue : public TaskQueue
{
public:
    SerialTaskQueue()
        : TaskQueue()
        , localThread(std::bind(&SerialTaskQueue::runLoop, this, std::placeholders::_1))
        , thread(localThread)
    {
        thread.start();
        setThreadId(thread.getId());
    }
    SerialTaskQueue(ThisThread& initThread)
        : TaskQueue()
        , thread(initThread)
    {
        initThread.setThreadProc(std::bind(&SerialTaskQueue::runLoop, this, std::placeholders::_1));
        setThreadId(thread.getId());
    }
    ~SerialTaskQueue()
    {
        setAcceptsTasks(false);
        if (thread.getIsStarted())
        {
            thread.stop();
        }
        queueWait.notify_all();
    }
    
protected:
    
    inline void notifyQueueChange() override
    {
        queueWait.notify_one();
    }
    
private:
    std::mutex waitMutex;
    std::condition_variable queueWait;
    Thread localThread;
    Thread& thread;
    
    inline void runLoop(const Thread::StopToken& stopToken)
    {
        while (!stopToken.getIsStopping())
        {
            std::unique_lock<std::mutex> lock(waitMutex);
            std::shared_ptr<Task> next;
            // Move delayed tasks to main queue
            const auto timeNow = std::chrono::steady_clock::now();
            auto nextTaskTime = enqueueDelayedTasks(timeNow);
            // Get the next task from the main queue
            {
                auto taskQueue = acquireTaskQueue();
                if (!taskQueue->empty())
                {
                    next = std::move(taskQueue->front());
                    taskQueue->pop();
                }
            }
            if (next)
            {
                next->execute();
            }
            else if (nextTaskTime != timeNow)
            {
                // There are no tasks to process, but delayedQueue had some tasks, we can wait till delay expires
                queueWait.wait_until(lock, nextTaskTime);
            }
            else if (getAcceptsTasks())
            {
                // We wait for a new task to be pushed on any of the queues
                queueWait.wait(lock);
            }
        }
        setAcceptsTasks(false);
        runLeftovers();
    }
    
    inline void runLeftovers()
    {
        // Process any leftover tasks
        auto taskQueue = acquireTaskQueue();
        while (taskQueue->size())
        {
            taskQueue->front()->execute();
            taskQueue->pop();
        }
    }
};

} // namespace gusc::Threads

#endif /* SerialTaskQueue_hpp */
