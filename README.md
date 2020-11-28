#  Thread and signal library

If you wish to:

* run a function on a specific thread 
* run callbacks that each are assigned to a different thread with thread afinity check

This is the library for you!

This is a C++17 library that provides threads with message queues and signal mechanism with full control in which thread the signal's listener will be executed.

## Thread with a message queue

Library provides an `std::thread` wrapper with an internal run-loop and a message queue. Whenever a message object get's sent to the thread, it's placed on a message queue and when time comes it's `operator()` method is executed in that thread.

This gives you options to post worker functions to be executed in different threads.

Example:

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
```

This will make the each lambda run on a different thread.

Additionally library provides a `MainThread` class to execute run-loop on current thread. This is intended to be used only on a main thread, but if you are running in a plug-in environment and wish to create your own run-loop, this might also help.

Example:

```c++
gusc::Threads::MainThread mt;

mt.send([&mt](){
    auto id = std::this_thread::get_id();
    std::cout << "This is a worker on thread ID: " << id << std::endl;
    
    // You can quit the main thread run-loop from anywhere
    mt.quit();
});

// Start the run-loop
mt.run();
```

This will run a run-loop in current thread and when the lambda is executed it will stop the run loop with the  `mt.quit()` call.

## Signals and slots

Library provides a Qt-style signal-slot functionality, but with standard C++ only.

Signals are means to emit data to multiple listeneres at once. All you have to do is to `connect()` to each signal with a target thread (`Thread*`) on which the callback should be executed and a callback function pointer itself.

If the listener is on the same thread where signal was emited from it's called directly and all the data is passed as `const&`. Data is only copied when signal is emitted to a different thread, then the data is packed together with the callback and placed on that threads message queue for later processing.

Example:

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

    gusc::Threads::MainThread mainThread;
    gusc::Threads::Thread workerThread;
    
    MyObject my;
    my.sigSimple.connect(&workerThread, [](){
        std::cout << "lambda thread ID: " << id << "\n";
    });
    my.sigSimple.connect(&mainThread, [](){
        std::cout << "lambda thread ID: " << id << "\n";
    });
    
    MyObject2 myThread(my);
    
    my.someMethod();
    
    mainThread.run();
    return 0;
}

```
