//
//  Signal.hpp
//  Threads
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#ifndef GUSC_SIGNAL_HPP
#define GUSC_SIGNAL_HPP

#include "TaskQueue.hpp"

#include <tuple>
#include <map>

namespace gusc
{
namespace Threads
{

/// @brief generic interface for opened signal connections
class SignalConnection
{
public:
    virtual ~SignalConnection() = default;
    virtual void close() {}
};

/// @brief class representing a signal connection and emission object
template<typename ...TArg>
class Signal
{
    /// @brief an internal abstract base class representing a signal callable wrapper
    class CallableBase
    {
    public:
        CallableBase() = default;
        virtual ~CallableBase() = default;

        virtual void operator()(const TArg&... args) = 0;

    };

    template<typename TCallable>
    class CallableWrapper : public CallableBase
    {
    public:
        CallableWrapper(const TCallable& initCallable)
            : callable(initCallable)
        {}

        inline void operator()(const TArg&... args) override
        {
            callable(args...);
        }

    private:
        TCallable callable;

    };
    
    /// @brief internal class representing a single message that wraps the signal data and the signal callable and is dispatched to a listener's queue
    class Message
    {
    public:
        Message(std::weak_ptr<CallableBase>&& initCallable, const TArg&... initData)
            : callable(initCallable)
            , data(initData...)
        {}

        inline void operator()()
        {
            if (auto c = callable.lock())
            {
                std::apply(*c, data);
            }
        }

    private:
        std::weak_ptr<CallableBase> callable;
        std::tuple<TArg...> data;

    };
    
    /// @brief internal class representing a signal connection slot (listener and it's affinity serial queue)
    class Slot
    {
    public:
        Slot() = delete;
        Slot(std::weak_ptr<TaskQueue> initHostQueue, std::shared_ptr<CallableBase>&& initCallable)
            : hostQueue(initHostQueue)
            , callable(std::move(initCallable))
        {}
        
        inline void call(const TArg&... args) const
        {
            auto queue = hostQueue.lock();
            if (!queue)
            {
                throw std::runtime_error("Host queue is gone");
            }
            if (queue->getIsSameThread())
            {
                (*callable)(args...);
            }
            else
            {
                queue->send(Message{callable, args...});
            }
        }
        
    private:
        std::weak_ptr<TaskQueue> hostQueue;
        std::shared_ptr<CallableBase> callable;

    };
    
public:
    Signal() = default;
    Signal(const Signal<TArg...>&) = delete;
    Signal& operator=(const Signal<TArg...>&) = delete;
    Signal(Signal<TArg...>&& other) = delete;
    Signal& operator=(Signal<TArg...>&& other) = delete;
    ~Signal()
    {
        for (auto& c : connections)
        {
            c->close();
        }
    }
    
    class Connection : public SignalConnection
    {
    public:
        Connection(Signal* initSignal, std::weak_ptr<Slot> initSlot)
            : signal(initSignal)
            , slot(initSlot)
        {
            if (signal)
            {
                signal->registerConnection(this);
            }
        }
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&& other)
            : signal(other.signal)
            , slot(std::move(other.slot))
        {
            if (signal)
            {
                signal->unregisterConnection(&other);
                signal->registerConnection(this);
            }
            other.signal = nullptr;
        }
        Connection& operator=(Connection&& other)
        {
            signal = other.signal;
            if (signal)
            {
                signal->unregisterConnection(&other);
                signal->registerConnection(this);
            }
            slot = std::move(other.slot);
            other.signal = nullptr;
            return *this;
        }
        ~Connection() override
        {
            close();
        }
        
        /// @brief close the signal connection
        inline void close() override
        {
            if (signal)
            {
                signal->disconnect(slot);
                signal->unregisterConnection(this);
                signal = nullptr;
            }
        }

    private:
        Signal* signal { nullptr };
        std::weak_ptr<Slot> slot;

    };
    
    /// @brief connect a listener callback to this signal
    /// @param queue - listener's task queue
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return connection object that holds the conection open until it's destroyed or Signal::Conenction::close() method is explicitly called
    template<typename TCallable>
    inline std::unique_ptr<Connection> connect(std::weak_ptr<TaskQueue> queue, const TCallable& callback) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        std::shared_ptr<CallableBase> callable = std::make_shared<CallableWrapper<TCallable>>(callback);
        auto& slot = slots.emplace_back(std::make_shared<Slot>(queue, std::move(callable)));
        return std::make_unique<Connection>(this, slot);
    }
    
    /// @brief disconnect all listeners from this signal
    inline void disconnectAll() noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        slots.clear();
    }
    
    /// @brief emit the signal to all of it's listeneres
    /// @param data - signal arguments
    inline void emit(const TArg&... data) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        for (const auto& slot : slots)
        {
            // TODO: think of ways to prevent locking emitMutex while calling
            try
            {
                slot->call(data...);
            }
            catch (...)
            {}
        }
    }
    
private:
    std::vector<std::shared_ptr<Slot>> slots;
    std::mutex emitMutex;
    std::vector<Connection*> connections;

    inline void registerConnection(Connection* connection)
    {
        if (std::find(connections.begin(), connections.end(), connection) == connections.end())
        {
            connections.push_back(connection);
        }
    }
    
    inline void unregisterConnection(Connection* connection)
    {
        auto it = std::find(connections.begin(), connections.end(), connection);
        if (it != connections.end())
        {
            connections.erase(it);
        }
    }
    
    inline bool disconnect(std::weak_ptr<Slot> slot) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        const auto it = std::find_if(slots.begin(), slots.end(), [&](const std::shared_ptr<Slot>& a){
            return !slot.expired() && slot.lock() == a;
        });
        if (it != slots.end())
        {
            slots.erase(it);
            return true;
        }
        return false;
    }

};

/// @brief template specialization for void arguments - unfortunatelly vararg templates can't handle that
template<>
class Signal<void>
{
    /// @brief an internal abstract base class representing a signal callable wrapper
    class CallableBase
    {
    public:
        CallableBase() = default;
        virtual ~CallableBase() = default;

        virtual void operator()() = 0;

    };

    template<typename TCallable>
    class CallableWrapper : public CallableBase
    {
    public:
        CallableWrapper(const TCallable& initCallable)
            : callable(initCallable)
        {}

        inline void operator()() override
        {
            callable();
        }

    private:
        TCallable callable;

    };
    
    /// @brief internal class representing a single message that wraps the signal data and the signal callable and is dispatched to a listener's queue
    class Message
    {
    public:
        Message(std::weak_ptr<CallableBase>&& initCallable)
            : callable(initCallable)
        {}

        inline void operator()()
        {
            if (auto c = callable.lock())
            {
                (*c)();
            }
        }

    private:
        std::weak_ptr<CallableBase> callable;

    };
    
    /// @brief internal class representing a signal connection slot (listener and it's affinity serial queue)
    class Slot
    {
    public:
        Slot() = delete;
        Slot(std::weak_ptr<TaskQueue> initHostQueue, std::shared_ptr<CallableBase>&& initCallable)
            : hostQueue(initHostQueue)
            , callable(std::move(initCallable))
        {}
        
        inline void call() const
        {
            auto queue = hostQueue.lock();
            if (!queue)
            {
                throw std::runtime_error("Host queue is gone");
            }
            if (queue->getIsSameThread())
            {
                (*callable)();
            }
            else
            {
                queue->send(Message{callable});
            }
        }
        
    private:
        std::weak_ptr<TaskQueue> hostQueue;
        std::shared_ptr<CallableBase> callable;

    };

    
public:
    Signal() = default;
    Signal(const Signal<void>&) = delete;
    Signal& operator=(const Signal<void>&) = delete;
    Signal(Signal<void>&& other) = delete;
    Signal& operator=(Signal<void>&& other) = delete;
    ~Signal()
    {
        for (auto& c : connections)
        {
            c->close();
        }
    }
    
    class Connection : public SignalConnection
    {
    public:
        Connection(Signal* initSignal, std::weak_ptr<Slot> initSlot)
            : signal(initSignal)
            , slot(initSlot)
        {
            if (signal)
            {
                signal->registerConnection(this);
            }
        }
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&& other)
            : signal(other.signal)
            , slot(std::move(other.slot))
        {
            if (signal)
            {
                signal->unregisterConnection(&other);
                signal->registerConnection(this);
            }
            other.signal = nullptr;
        }
        Connection& operator=(Connection&& other)
        {
            signal = other.signal;
            if (signal)
            {
                signal->unregisterConnection(&other);
                signal->registerConnection(this);
            }
            slot = std::move(other.slot);
            other.signal = nullptr;
            return *this;
        }
        ~Connection() override
        {
            close();
        }
        
        /// @brief close the signal connection
        inline void close() override
        {
            if (signal)
            {
                signal->disconnect(slot);
                signal->unregisterConnection(this);
                signal = nullptr;
            }
        }

    private:
        Signal* signal { nullptr };
        std::weak_ptr<Slot> slot;

    };
    
    friend Connection;
    
    /// @brief connect a listener callback to this signal
    /// @param queue - listener's task queue
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return connection object that holds the conection open until it's destroyed or Signal::Conenction::close() method is explicitly called
    template<typename TCallable>
    inline std::unique_ptr<Connection> connect(std::weak_ptr<TaskQueue> queue, const TCallable& callback) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        std::shared_ptr<CallableBase> callable = std::make_shared<CallableWrapper<TCallable>>(callback);
        auto& slot = slots.emplace_back(std::make_shared<Slot>(queue, std::move(callable)));
        return std::make_unique<Connection>(this, slot);
    }
    
    /// @brief disconnect all listeners from this signal
    inline void disconnectAll() noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        slots.clear();
    }
    
    /// @brief emit the signal to all of it's listeneres
    /// @param data - signal arguments
    inline void emit() noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        for (const auto& slot : slots)
        {
            // TODO: think of ways to prevent locking emitMutex while 
            try
            {
                slot->call();
            }
            catch (...)
            {}
        }
    }

private:
    std::vector<std::shared_ptr<Slot>> slots;
    std::mutex emitMutex;
    std::vector<Connection*> connections;

    inline void registerConnection(Connection* connection)
    {
        if (std::find(connections.begin(), connections.end(), connection) == connections.end())
        {
            connections.push_back(connection);
        }
    }
    
    inline void unregisterConnection(Connection* connection)
    {
        auto it = std::find(connections.begin(), connections.end(), connection);
        if (it != connections.end())
        {
            connections.erase(it);
        }
    }
    
    inline bool disconnect(std::weak_ptr<Slot> slot) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        const auto it = std::find_if(slots.begin(), slots.end(), [&](const std::shared_ptr<Slot>& a){
            return !slot.expired() && slot.lock() == a;
        });
        if (it != slots.end())
        {
            slots.erase(it);
            return true;
        }
        return false;
    }
    
};

}
} // namespace gusc::Threads
    
#endif /* GUSC_SIGNAL_HPP */
