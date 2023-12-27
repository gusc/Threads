#  Thread and signal library

If you wish to:

* run a function on a specific thread 
* run callbacks that each are assigned to a different thread with thread affinity check
* have greater control over which code accesses which data in which thread
* post tasks on queues associated with threads

Then this is the library for you!

This is a C++17 library that provides threads with message queues and signal mechanism with full control in which thread the signal's listener will be executed.

Table of contents:

* [Thread class](#thread-class)
    * [ThisThread class](#thisthread-class)
    * [ThreadPool class](#threadpool-class)
    * [Examples](#examples)
* [TaskQueue class](#taskqueue-class)
    * [SerialTaskQueue class](#serialtaskqueue-class)
    * [ParallelTaskQueue class](#paralleltaskqueue-class)
    * [Examples](#examples-1)
* [Signals with listener slots](#signals-with-listener-slots)
    * [Signal class](#signal-class)
    * [Examples](#examples-2)

## Thread class

Library provides an `std::thread` wrapper with:

* delayed start - you can set up your thread and start it later
* stop signaling - your thread procedure can receive a `const Thread::StopToken&` via which you can check whether someone has signaled the thread to stop
* start signaling - `Thread::start` returns a `Thread::StartToken&` which allows you to wait until thread procedure has actually started

`Thread` methods:

* `Thread(const std::string& threadName, TFunction&&, TArgs&&...)` - constructor accepting any kind of callable objects and additional arguments (there is a special case for function that accepts `[const ]Thread::StopToken[&]` as it's first argument)
* `Thread::StartToken& start()` - start running the thread, returns a start token which can be used to wait for the thread procedure to actually start
* `void stop()` - signal the thread to stop - this will signal the `Thread::StopToken` which you can then check on your thread procedure via `Thread::StopToken::getIsStopping()` method
* `void join()` - join the thread if it has started and is joinable
* `std::thread::id getId()` - get wrapped thread ID (if the thread is started)
* `bool getIsStarted()` - check whether the thread has been started (this does not mean the thread procedure has actually started running)
* `bool getIsStopping()` - check whether the thread has been signaled for stopping (this does not mean the thread procedure has actually stopped)

`Thread` class automatically joins on destruction.

### ThisThread class

Additionally library provides a `ThisThread` class to execute procedure in `Thread` context. This is intended to be used only on a main thread or any other current thread that was not started by `Thread` class to use in a similar manner as `Thread` objects.

`ThisThread` extends from `Thread` and starts thread procedure when `start()` is called effectivelly blocking current thread until someone calls `stop()` and your thread procedure handles stopping signal.

### ThreadPool class

Thread pool allows running single callable object on multiple threads.

`ThreadPool` methods:

* `ThreadPool(std::string threadName, std::size_t threadCount, TFunction&&, TArgs&&...)` - constructor accepting any kind of callable objects and additional arguments (there is a special case for function that accepts `[const ]Thread::StopToken[&]` as it's first argument)
* `void resize(std::size_t threadCount)` resize the thread pool (only invocable while thread pool is not running, otherwise an exception will be thrown)
* `void start()` - start running the thread
* `void stop()` - signal the thread to stop - this will signal the `Thread::StopToken` which you can then check on your thread procedure via `Thread::StopToken::getIsStopping()` method
* `std::size_t getSize()` - get number of threads in the pool
* `bool getIsThreadIdInPool(std::thread::id threadId)` - check if thread ID provided is in the pool

### Examples

For actual real-world usage examples see [Examples directory](./Examples) and [Tests directory](./Tests)

## TaskQueue class

This library provides a task queue which allows posting more complex tasks on a thread that includes:

* tasks that are simply run whenever thread becomes free
* tasks that return a handle from which you can cancel, check if task has executed and even get an asychronous results
* tasks that can block current thread and wait until it's executed 

`TaskQueue` is a base class for any style of queueing mechanism. The final implementation is `SerialTaskQueue` which runs on a single thread and processes tasks in-order (there is a `ParallelTaskQueue` planned that will use `ThreadPool` which is also WIP).

`TaskQueue` constructors:

* `TaskQueue(const std::function<void(void)>& initQueueNotifyCallback)` - construct new task queue with notify callback, the callback get's called whenever a task queue changes it's contents

`TaskQueue` task methods:

* `void send(const TCallable&)` - place a callabable object on the task queue
* `TaskHandle sendDelayed(const TCallable&, const std::chrono:milliseconds&)` - place a callabable object on the message queue and execute it after set delay time has elapsed (this method also returns a `TaskHandle` object that allows to cancel the message while it's delay hasn't elapsed).
* `TaskHandleWithResult<TReturn> sendAsync<TReturn>(const TCallable&)` - place a callabable object that can return value asynchronously on the task queue (this message return `TaskHandleWithResult<TReturn>` - similar to `TaskHandle`, but it can also be use to block current thread until the task has finished or exception has occured.
* `TReturn sendSync<TReturn>(const TCallable&)` - place a callabable object that can return value synchronously on the task queue (this blocks calling thread until the callable finishes and returns)
* `void sendWait(const TCallable&)` - place a callable object on the task queue and block until it's executed queue
* `void cancelAll()` - cancel all pending tasks

Sub-queue creation methods:

* `std::shared_ptr<TaskQueue> createSubQueue()` - create new queue that acts as a sub-queue of current queue

Notes about sub-queues:

1. all the tasks placed in sub-queue will be processed in the parent queues thread
2. delayed tasks will be moved to parent queue only after delay time has elapsed
3. on destruction all tasks are cancelled automatically (as opposed to tasks assigned to main queue)

Utility methods:

* `bool getIsSameThread()` - check if we are accessing this queue on the same thread as the queue itself
* `bool getAcceptsTasks()` - check if task queue is accepting new tasks (it might not accept tasks if it's not started or is stopped)

`TaskQueue` class always finishes all the tasks on the queue on destruction and cancels all the delayed tasks.

*Blocking call warning*: Sending a blocking message on a task queue that is not started will result in an exception!

`TaskHandle` methods:

* `void cancel()` - cancel task if it's not yet started 

`TaskHandleWithFuture<TResult>` methods:

* `void cancel()` - cancel task if it's not yet started 
* `TResult getValue()` - acquire the return value of the task (blocks and waits if task has not been finished yet)

### SerialTaskQueue class

The implementation of serial task queue is based on `TaskQueue` with serial run-loop logic.

* `SerialTaskQueue(const std::string& queueName)` - construct a new serial task queue
* `SerialTaskQueue(ThisThread& initThread)` - special constructor to place serial task queue on `ThisThread`

### ParallelTaskQueue class

The implementation of parallel task queue is based on `TaskQueue` with concurrent job-stealing run-loop logic runing on a `ThreadPool`.

* `ParallelTaskQueue(const std::string& queueName, std::size_t queueCount)` - construct a new parallel task queue

### Examples

For actual real-world usage examples see [Examples directory](./Examples) and [Tests directory](./Tests)

## Signals with listener slots

Library provides a Qt-style signal-slot functionality, but with standard C++ only.

Signals are means to emit data to multiple listeneres at once. All you have to do is to `connect()` to each signal with a target thread (`Thread*`) on which the callback should be executed and a callback function pointer itself.

If the listener is on the same thread where signal was emited from it's called directly and all the data is passed as `const&`. Data is only copied when signal is emitted to a different thread, then the data is packed together with the callback and placed on that threads message queue for later processing.

### Signal class

It's a templated class who's method signature depend on what arguments the signal has.

`Signal` methods:

* `size_t connect(Thread*, const std::function<void(TArg...)>&)` - connect a listener to the signal (returns the connection ID or 0 if failed to store the connection)
* `size_t connect(T*, const void(T::*)(TArg...))` - connect a listener member method of Thread class derivative to the signal (returns the connection ID or 0 if failed to store the connection)
* `bool disconnect(Thread*, const std::function<void(TArg...)>&)` - disconnect a listener from the signal (returns false if function/thread pair is not found or if function came from temporary object, like std::bind)
* `bool disconnect(T*, const void(T::*)(TArg...))` - disconnect a listener member method of Thread class derivative from the signal (returns false if function/thread is not found in connection list)
* `bool disconnect(const size_t)` - disconnect a listener from the signal by connection ID (returns false if ID not found in connection list)
* `void emit(const TArg&...)` - emit the signal with data - this will call all the connected listeners on their respecitve affinity threads

When disconnecting listeners from signals, for function objects, like ones returned by `std::bind` or lambdas, you should use connection ID's.

### Examples

```c++

class MyObject final
{
public:
    MyObject()
    {}
    
    public someMethod()
    {
        sigSimple.emit();
        sigArgs.emit(1, 2);
    }
    
    // Signal definition includes signal argument types
    gusc::Threads::Signal<void> sigSimple;
    gusc::Threads::Signal<int, int> sigArgs;
};

class MyObject2 final : gusc::Threads::Thread
{
public:
    MyObject2(MyObject& initOther)
        : Thread()
    {
        initOther.sigArgs.connect(this, &MyObject2::onArgs)
    }
    
private:
    void onArgs(int a, int b)
    {
        auto id = std::this_thread::get_id();
        std::cout << "MyObject2::onArgs thread ID: " << id << "\n";
    }    
};

int main(int argc, const char * argv[]) {
    auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << "\n";

    gusc::Threads::ThisThread mainThread;
    gusc::Threads::Thread workerThread;
    
    MyObject my;
    const auto connection1 = my.sigSimple.connect(&workerThread, [](){
        std::cout << "lambda thread ID: " << id << "\n";
    });
    const auto connection2 = my.sigSimple.connect(&mainThread, [](){
        std::cout << "lambda thread ID: " << id << "\n";
    });
    
    MyObject2 myThread(my);
    
    my.someMethod(); // Emits the signals
    
    // We need to start our thread so that their signal listeners are executed
    workerThread.start();
    myThread.start(); 
    mainThread.start(); // Blocks indefinitely
    
    my.sigSimple.disconnect(connection1);
    my.sigSimple.disconnect(connection2);
    return 0;
}

```
