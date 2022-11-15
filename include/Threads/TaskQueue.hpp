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
    
    TaskQueue(const std::function<void(void)>& initQueueNotifyCallback)
        : queueNotifyCallback(initQueueNotifyCallback)
    {}
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    TaskQueue(TaskQueue&&) = delete;
    TaskQueue& operator=(TaskQueue&&) = delete;
    ~TaskQueue()
    {
        setAcceptsTasks(false);
    }

    /// @brief send a task that needs to be executed on this thread
    /// @param newTask - any callable object that will be executed on this thread
    template<typename TCallable>
    inline void send(const TCallable& newTask)
    {
        if (getAcceptsTasks())
        {
            const std::lock_guard<decltype(mutex)> lock(mutex);
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
            const std::lock_guard<decltype(mutex)> lock(mutex);
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
                const std::lock_guard<decltype(mutex)> lock(mutex);
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
    
    /// @brief Create a sub-queue that who's ownership will be transfered to the caller
    inline std::shared_ptr<TaskQueue> createSubQueue()
    {
        auto subQueue = std::make_shared<TaskQueue>([this](){
            notifyQueueChange();
        });
        subQueue->setThreadId(threadId);
        subQueue->setAcceptsTasks(getAcceptsTasks());
        subQueues.push_back(subQueue);
        return subQueue;
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
    
    /// @brief Cancel all the tasks
    inline void cancelAll() noexcept
    {
        const std::lock_guard<decltype(mutex)> lock(mutex);
        setAcceptsTasks(false);
        delayedQueue.clear();
        while (taskQueue.size())
        {
            taskQueue.pop();
        }
        for (auto& q : subQueues)
        {
            if (auto queue = q.lock())
            {
                queue->cancelAll();
            }
        }
    }
    
protected:
    /// @brief base class for thread task
    class Task
    {
    public:
        virtual ~Task() = default;
        inline void execute() {
            const std::lock_guard<decltype(mutex)> lock(mutex);
            if (!isCancelled)
            {
                privateExecute();
            }
            isExecuted = true;
        }
        inline void cancel() noexcept
        {
            const std::lock_guard<decltype(mutex)> lock(mutex);
            if (!isCancelled && !isExecuted)
            {
                isCancelled = true;
                privateCancel();
            }
        }
    protected:
        
        virtual void privateExecute() = 0;
        virtual void privateCancel() = 0;
        
    private:
        std::recursive_mutex mutex;
        bool isCancelled { false };
        bool isExecuted { false };
    };
    
    /// @brief templated task to wrap a callable object
    template<typename TCallable>
    class TaskWithCallable : public Task
    {
    public:
        TaskWithCallable(const TCallable& initCallableObject)
            : callableObject(initCallableObject)
        {}
        ~TaskWithCallable() override
        {
            cancel();
        }
    protected:
        inline void privateExecute() override
        {
            callableObject();
        }
        inline void privateCancel() override
        {}
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
        ~TaskWithPromise() override
        {
            cancel();
        }
    protected:
        inline void privateExecute() override
        {
            privateExecute<TReturn>();
        }
        inline void privateCancel() override
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
    private:
        TCallable callableObject;
        std::promise<TReturn> waitablePromise;
        
        template<typename T>
        inline void privateExecute()
        {
            try
            {
                waitablePromise.set_value(callableObject());
            }
            catch(...)
            {
                waitablePromise.set_exception(std::current_exception());
            }
        }
        
        template<>
        inline void privateExecute<void>()
        {
            try
            {
                callableObject();
                waitablePromise.set_value();
            }
            catch(...)
            {
                waitablePromise.set_exception(std::current_exception());
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
        auto timeNext = timeNow;
        if (!delayedQueue.empty())
        {
            timeNext = (*delayedQueue.begin())->getTime();
        }
        // Check for sub-queue closest delayed task deadline
        for (auto q : subQueues)
        {
            if (auto queue = q.lock())
            {
                auto nextTime = queue->enqueueDelayedTasks(timeNow);
                if (nextTime < timeNext)
                {
                    timeNext = nextTime;
                }
            }
        }
        return timeNext;
    }
    
    inline void setThreadId(std::thread::id newThreadId)
    {
        const std::lock_guard<decltype(mutex)> lock(mutex);
        threadId = newThreadId;
        for (auto& q : subQueues)
        {
            if (auto queue = q.lock())
            {
                queue->setThreadId(newThreadId);
            }
        }
    }
    
    inline void setAcceptsTasks(bool newAcceptsTasks)
    {
        const std::lock_guard<decltype(mutex)> lock(mutex);
        acceptsTasks = newAcceptsTasks;
        for (auto& q : subQueues)
        {
            if (auto queue = q.lock())
            {
                queue->setAcceptsTasks(newAcceptsTasks);
            }
        }
    }
    
    inline void notifyQueueChange()
    {
        if (queueNotifyCallback)
        {
            queueNotifyCallback();
        }
    }
    
    inline LockedReference<std::queue<std::shared_ptr<Task>>, std::mutex> acquireTaskQueue()
    {
        {
            // Move all the subqueue tasks to this task queue
            const std::lock_guard<decltype(mutex)> lock(mutex);
            auto it = subQueues.begin();
            while (it != subQueues.end())
            {
                if (auto queue = it->lock())
                {
                    auto tasks = queue->acquireTaskQueue();
                    while (tasks->size())
                    {
                        taskQueue.emplace(std::move(tasks->front()));
                        tasks->pop();
                    }
                    ++it;
                }
                else
                {
                    // Remove sub-queue that might be destroyed by it's owner
                    it = subQueues.erase(it);
                }
            }
        }
        return { taskQueue, mutex };
    }
    
private:
    std::mutex mutex;
    std::thread::id threadId { std::this_thread::get_id() };
    std::atomic_bool acceptsTasks { true };
    std::queue<std::shared_ptr<Task>> taskQueue;
    std::multiset<std::unique_ptr<DelayedTaskWrapper>> delayedQueue;
    std::vector<std::weak_ptr<TaskQueue>> subQueues;
    std::function<void(void)> queueNotifyCallback { nullptr };
};

/// @brief a class representing a task queue that's running serially on a single thread
class SerialTaskQueue : public TaskQueue
{
public:
    SerialTaskQueue()
        : TaskQueue([this](){
            queueWait.notify_one();
        })
        , localThread(std::bind(&SerialTaskQueue::runLoop, this, std::placeholders::_1))
        , thread(localThread)
    {
        thread.start();
        setThreadId(thread.getId());
    }
    SerialTaskQueue(ThisThread& initThread)
        : TaskQueue([this](){
            queueWait.notify_one();
        })
        , thread(initThread)
    {
        initThread.setThreadProcedure(std::bind(&SerialTaskQueue::runLoop, this, std::placeholders::_1));
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
                try
                {
                    next->execute();
                }
                catch (...)
                {
                    // We can't do nothing as nobody is listening, but we don't want the thread to explode
                }
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
