#  Thread and signal library

If you wish to:

* run a function on a specific thread 
* run callbacks that each are assigned to a different thread with thread affinity check
* have greater control over which code accesses which data in which thread

Then this is the library for you!

This is a C++17 library that provides threads with message queues and signal mechanism with full control in which thread the signal's listener will be executed.

## Thread with a message queue

Library provides an `std::thread` wrapper with an internal run-loop and a message queue. Whenever a message object get's sent to the thread, it's placed on a message queue and when time comes it's `operator()` method is executed in that thread.

This gives you options to post worker functions to be executed in different threads.

### Thread class

`Thread` methods:

* `void send(const TCallable&)` - place a callabable object on the message queue
* `void sendDelayed(const TCallable&, const std::chrono:milliseconds&)` - place a callabable object on the message queue and execute it after set delay time has elapsed
* `void start()` - start running the thread (also automatically start run-loop)
* `void stop()` - signal the thread to stop - this will make the thread stop accepting new messages, but it will still continue processing messages in the queue
* `void join()` - wait for the thread to finish

`Thread` class automatically joins on destruction.

### ThisThread class

Additionally library provides a `ThisThread` class to execute run-loop on current thread. This is intended to be used only on a main thread or any other thread that was not started by `Thread` class.

`ThisThread` extends from `Thread` and starts run loop when `start()` is called effectivelly blocking current thread until someone calls `stop()` (which can be done either through a message or before calling `start()`).

### Examples

This will make the each lambda run on a different thread:

```c++
gusc::Threads::Thread th1;
gusc::Threads::Thread th2;

th1.send([](){
    auto id = std::this_thread::get_id();
    std::cout << "This is a worker on thread ID: " << id << std::endl;
});

th2.send([](){
    auto id = std::this_thread::get_id();
    std::cout << "This is a worker on thread ID: " << id << std::endl;
});

auto id = std::this_thread::get_id();
std::cout << "Main thread ID: " << id << std::endl;

// You have to start threads to process all the messages
th1.start();
th2.start();
// Both threads will be joined up on destruction
```

Example with `ThisThread` - this will run a run-loop in current thread and when the lambda is executed it will stop the run loop with the  `mt.stop()` call.

```c++
gusc::Threads::ThisThread mt;

mt.send([&mt](){
    auto id = std::this_thread::get_id();
    std::cout << "This is a worker on thread ID: " << id << std::endl;
    
    // You can quit the main thread run-loop from anywhere
    mt.stop();
});

// Start the run-loop
mt.start();
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
