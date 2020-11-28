#  Thread and signal library

This is a C++17 library that provides threads with message queues and signal mechanism with full control in which thread the signal's listener will be executed.

# Thread with a message queue

Library provides an `std::thread` wrapper with an internal run-loop and a message queue. Whenever a message object get's posted on the queue it's `operator()` method is executed in the thread.

This gives you options to post callback objects to be executed in different threads.

Example:

```c++
Thread th;
th.send([](){
    auto id = std::this_thread::get_id();
    std::cout << "Thread ID: " << id << std::endl;
});

auto id = std::this_thread::get_id();
std::cout << "Thread ID: " << id << std::endl;
```

# Signals and slots

Library provides a Qt-style signal-slot functionality, but with only standard C++ and the thread message queue.

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

    MainThread mainThread;
    Thread workerThread;
    
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
