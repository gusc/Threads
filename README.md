#  Thread and signal library

If you wish to:

* run a function on a specific thread 
* run callbacks that each are assigned to a different thread with thread affinity check
* have greater control over which code accesses which data in which thread
* post tasks on queues associated with threads

Then this is the library for you!

This is a C++17 library that provides threads with message queues and signal mechanism with full control in which thread the signal's listener will be executed.

## Thread class

Library provides an `std::thread` wrapper with an internal run-loop and a callable object queue. Whenever a callable object get's sent to the thread, it's placed on a queue and when time comes it's `operator()` method is executed in that thread.

This gives you options to post worker functions to be executed in different threads.

`Thread` methods:

* `void run(const TCallable&)` - place a callabable object on the runnable queue (the thread must be started!)
* `void start()` - start running the thread, also start accepting new runnables
* `void stop()` - signal the thread to stop - this will make the thread stop accepting new runnables, but it will still continue processing messages in the queue
* `void join()` - wait for the thread to finish

`Thread` class automatically joins on destruction.

## ThisThread class

Additionally library provides a `ThisThread` class to execute run-loop on current thread. This is intended to be used only on a main thread or any other thread that was not started by `Thread` class.

`ThisThread` extends from `Thread` and starts run loop when `start()` is called effectivelly blocking current thread until someone calls `stop()` (which can be done either through a message or before calling `start()`).

## Examples

This will make the each lambda run on a different thread:

```c++
{
    gusc::Threads::Thread th1;
    gusc::Threads::Thread th2;
    
    auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << std::endl;
    
    // You have to start threads to process all the messages
    th1.start();
    th2.start();
    
    // Place messages on both threads
    th1.run([](){
        auto id = std::this_thread::get_id();
        std::cout << "This is a worker on thread ID: " << id << std::endl;
    });
    
    th2.run([](){
        auto id = std::this_thread::get_id();
        std::cout << "This is a worker on thread ID: " << id << std::endl;
    });
    
    // Both threads will be joined up on thread object destruction
}
```

Example with `ThisThread` - this will run a run-loop in current thread and when the lambda is executed it will stop the run loop with the  `mt.stop()` call.

```c++
gusc::Threads::ThisThread mt;

mt.run([&mt](){
    auto id = std::this_thread::get_id();
    std::cout << "This is a worker on thread ID: " << id << std::endl;
    
    // You can quit the main thread run-loop from anywhere
    mt.stop();
});

// Start the run-loop
mt.start();
```

## Task Queue

This library provides a task queue which allows posting more complex tasks on a thread that includes:

* tasks that are simply run whenever thread becomes free
* tasks that return a handle from which you can cancel, check if task has executed and even get an asychronous results
* tasks that can block current thread and wait until it's executed 

`TaskQueue` methods:

* `void send(const TCallable&)` - place a callabable object on the task queue
* `TaskHandle sendDelayed(const TCallable&, const std::chrono:milliseconds&)` - place a callabable object on the message queue and execute it after set delay time has elapsed (this method also returns a `TaskHandle` object that allows to cancel the message while it's delay hasn't elapsed).
* `TaskHandleWithResult<TReturn> sendAsync<TReturn>(const TCallable&)` - place a callabable object that can return value asynchronously on the task queue (this message return `TaskHandleWithResult<TReturn>` - similar to `TaskHandle`, but it can also be use to block current thread until the task has finished or exception has occured.
* `TReturn sendSync<TReturn>(const TCallable&)` - place a callabable object that can return value synchronously on the task queue (this blocks calling thread until the callable finishes and returns)
* `void sendWait(const TCallable&)` - place a callable object on the task queue and block until it's executed
* `void start()` - start running the task queue
* `void stop()` - signal the thread to stop - this will make the task queue stop accepting new messages, but it will still continue processing messages in the queue

`TaskQueue` class always finishes all the tasks on the queue on destruction and cancels all the delayed tasks.

*Blocking call warning*: Sending a blocking message on a task queue that is not started will result in an exception!

## Examples

This will make the each lambda run on a different thread:

```c++
{
    gusc::Threads::TaskQueue tq;
    
    auto id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << id << std::endl;
    
    // You have to start threads to process all the messages
    tq.start();
    
    // Place messages on both threads
    tq.send([](){
        auto id = std::this_thread::get_id();
        std::cout << "This is a worker on thread ID: " << id << std::endl;
    });
    
    th2.send([](){
        auto id = std::this_thread::get_id();
        std::cout << "This is a worker on thread ID: " << id << std::endl;
    });
    
    // Place a synchronous message - this blocks current thread until the th1 finishes this miessage
    th1.sendWait([](){
       
    });
    
    // Place a message returning a synchronous result
    auto result1 = th1.sendSync<int>([]() -> int {
        return 1;
    });
    
    // Place a message returning an asynchronous result via std::future
    auto f = th2.sendAsync<bool>([]() -> bool {
        return true;
    });
    auto result2 = f.getValue();
}
```

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
