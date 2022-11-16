//
//  Signal.hpp
//  Threads
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#ifndef Signal_hpp
#define Signal_hpp

#include "TaskQueue.hpp"

#include <tuple>
#include <map>

namespace gusc::Threads
{

/// @brief class representing a signal connection and emission object
template<typename ...TArg>
class Signal
{
    /// @brief internal class representing a single message that wraps the signal data and the listener and is dispatched to a listener's queue
    class SignalMessage
    {
    public:
        SignalMessage(const std::function<void(TArg...)>& initCallback, const TArg&... initData)
            : callback(initCallback)
            , data(initData...)
        {}
        inline void operator()()
        {
            std::apply(callback, data);
        }
    private:
        std::function<void(TArg...)> callback;
        std::tuple<TArg...> data;
    };
    
    /// @brief internal class representing a signal connection slot (listener and it's affinity serial queue)
    class Slot
    {
    public:
        Slot() = delete;
        Slot(SerialTaskQueue* initHostQueue, void* initCallbackPtr, const std::function<void(TArg...)>& initCallback)
            : hostQueue(initHostQueue)
            , callbackPtr(initCallbackPtr)
            , callback(initCallback)
        {}
        
        inline void setConnectionId(size_t newConnectionId) noexcept
        {
            connectionId = newConnectionId;
        }
        
        inline size_t getConnectionId() const noexcept
        {
            return connectionId;
        }
        
        inline bool operator==(const Slot& other) const noexcept
        {
            return callbackPtr && hostQueue == other.hostQueue && callbackPtr != other.callbackPtr;
        }
        
        inline void call(const TArg&... args) const
        {
            if (!hostQueue)
            {
                throw std::runtime_error("Host queue is null");
            }
            if (hostQueue->getIsSameThread())
            {
                callback(args...);
            }
            else
            {
                hostQueue->send(SignalMessage{callback, args...});
            }
        }
        
    private:
        SerialTaskQueue* hostQueue { nullptr };
        void* callbackPtr { nullptr };
        std::function<void(TArg...)> callback;
        size_t connectionId { 0 };

    };
    
public:
    Signal() = default;
    Signal(const Signal<TArg...>&) = delete;
    Signal& operator=(const Signal<TArg...>&) = delete;
    Signal(Signal<TArg...>&& other) = delete;
    Signal& operator=(Signal<TArg...>&& other) = delete;
    ~Signal() = default;
    
    /// @brief connect a listener callback to this signal
    /// @param queue - listener's serial task queue
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return connection ID for disconnecting the slot later or 0 if failed to insert the slot
    inline size_t connect(SerialTaskQueue* queue, const std::function<void(const TArg&...)>& callback) noexcept
    {
        typedef void(fnType)(const TArg&...);
        fnType* const* fnPointer = callback.template target<fnType*>();
        return connect({queue, fnPointer ? reinterpret_cast<void*>(*fnPointer) : nullptr, callback});
    }

    /// @brief connect a listener callback to this signal
    /// @param queue - listener's serial task queue
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return connection ID for disconnecting the slot later or 0 if failed to insert the slot
    template<typename TClass>
    inline size_t connect(TClass* queue, void(TClass::* callback)(const TArg&...)) noexcept
    {
        return connect(Slot{queue, reinterpret_cast<void*&>(callback), [queue, callback](const TArg&... args){(queue->*callback)(args...);}});
    }

    /// @brief connect a listener callback to this signal
    /// @param queue - listener's serial task queue
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return connection ID for disconnecting the slot later or 0 if failed to insert the slot
    template<typename TClass>
    inline size_t connect(TClass* queue, void(TClass::* callback)(TArg...)) noexcept
    {
        return connect(Slot{queue, reinterpret_cast<void*&>(callback), [queue, callback](const TArg&... args){(queue->*callback)(args...);}});
    }
    
    /// @brief disconnect a listener callback from this signal
    /// @param queue - listener's serial task queue
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    inline bool disconnect(SerialTaskQueue* queue, const std::function<void(const TArg&...)>& callback) noexcept
    {
        typedef void(fnType)(const TArg&...);
        fnType* const* fnPointer = callback.template target<fnType*>();
        return disconnect({queue, fnPointer ? reinterpret_cast<void*>(*fnPointer) : nullptr, callback});
    }
    
    /// @brief disconnect a listener callback from this signal
    /// @param queue - listener's serial task queue
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    template<typename TClass>
    inline bool disconnect(TClass* queue, void(TClass::* callback)(const TArg&...)) noexcept
    {
        return disconnect(Slot{queue, reinterpret_cast<void*&>(callback), [queue, callback](const TArg&... args){(queue->*callback)(args...);}});
    }

    /// @brief disconnect a listener callback from this signal
    /// @param queue - listener's serial task queue
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    template<typename TClass>
    inline bool disconnect(TClass* queue, void(TClass::* callback)(TArg...)) noexcept
    {
        return disconnect(Slot{queue, reinterpret_cast<void*&>(callback), [queue, callback](const TArg&... args){(queue->*callback)(args...);}});
    }
    
    /// @brief disconnect a listener callback from this signal using it's connection ID
    /// @param connectionId - a connection ID assigned and returned from connect() call
    /// @return false if no listener with this connection ID was found
    inline bool disconnect(const size_t connectionId) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        const auto it = std::find_if(slots.begin(), slots.end(), [&connectionId](const Slot& s){
            return s.getConnectionId() == connectionId;
        });
        if (it != slots.end())
        {
            slots.erase(it);
            return true;
        }
        return false;
    }
    
    /// @brief emit the signal to all of it's listeneres
    /// @param data - signal arguments
    inline void emit(const TArg&... data) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        for (const auto& l : slots)
        {
            l.call(data...);
        }
    }
    
private:
    std::vector<Slot> slots;
    size_t uniqueIdCounter { 0 };
    std::mutex emitMutex;
    
    inline size_t connect(const Slot& slot) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        const auto it = std::find(slots.begin(), slots.end(), slot);
        if (it == slots.end())
        {
            auto& s = slots.emplace_back(slot);
            ++uniqueIdCounter;
            s.setConnectionId(uniqueIdCounter);
            return uniqueIdCounter;
        }
        else
        {
            return it->getConnectionId();
        }
        return 0;
    }
    
    inline bool disconnect(const Slot& slot) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        const auto it = std::find(slots.begin(), slots.end(), slot);
        if (it != slots.end())
        {
            const auto slotIndex = std::distance(slots.begin(), it);
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
    class Slot
    {
    public:
        Slot() = delete;
        Slot(SerialTaskQueue* initHostQueue, void* initCallbackPtr, const std::function<void(void)>& initCallback)
            : hostQueue(initHostQueue)
            , callbackPtr(initCallbackPtr)
            , callback(initCallback)
        {}
        
        inline void setConnectionId(size_t newConnectionId) noexcept
        {
            connectionId = newConnectionId;
        }
        
        inline size_t getConnectionId() const noexcept
        {
            return connectionId;
        }
        
        bool operator==(const Slot& other)
        {
            return callbackPtr && hostQueue == other.hostQueue && callbackPtr == other.callbackPtr;
        }
        
        void call() const
        {
            if (hostQueue->getIsSameThread())
            {
                callback();
            }
            else
            {
                hostQueue->send(callback);
            }
        }
        
    private:
        SerialTaskQueue* hostQueue { nullptr };
        void* callbackPtr { nullptr };
        std::function<void(void)> callback;
        size_t connectionId { 0 };

    };

    
public:
    Signal() = default;
    Signal(const Signal<void>&) = delete;
    Signal& operator=(const Signal<void>&) = delete;
    Signal(Signal<void>&& other) = delete;
    Signal& operator=(Signal<void>&& other) = delete;
    ~Signal() = default;
    
    inline bool connect(SerialTaskQueue* queue, const std::function<void(void)>& callback) noexcept
    {
        typedef void(fnType)(void);
        fnType* const* fnPointer = callback.target<fnType*>();
        return connect({queue, fnPointer ? reinterpret_cast<void*>(*fnPointer) : nullptr, callback});
    }
    
    template<typename TClass>
    inline bool connect(TClass* queue, void(TClass::* callback)(void)) noexcept
    {
        return connect(Slot{queue, reinterpret_cast<void*&>(callback), [queue, callback](){(queue->*callback)();}});
    }
    
    inline bool disconnect(SerialTaskQueue* queue, const std::function<void(void)>& callback) noexcept
    {
        typedef void(fnType)(void);
        fnType* const* fnPointer = callback.target<fnType*>();
        return disconnect({queue, fnPointer ? reinterpret_cast<void*>(*fnPointer) : nullptr, callback});
    }
    
    template<typename TClass>
    inline bool disconnect(TClass* queue, void(TClass::* callback)(void)) noexcept
    {
        return disconnect(Slot{queue, reinterpret_cast<void*&>(callback), [queue, callback](){(queue->*callback)();}});
    }

    inline bool disconnect(const size_t connectionId) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        const auto it = std::find_if(slots.begin(), slots.end(), [&connectionId](const Slot& s){
            return s.getConnectionId() == connectionId;
        });
        if (it != slots.end())
        {
            slots.erase(it);
            return true;
        }
        return false;
    }
    
    inline void emit() noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        for (auto& l : slots)
        {
            l.call();
        }
    }
    
private:
    std::vector<Slot> slots;
    size_t uniqueIdCounter { 0 };
    std::map<size_t, typename std::vector<Slot>::size_type> indexMap;
    std::mutex emitMutex;
    
    inline bool connect(const Slot& slot) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        const auto it = std::find(slots.begin(), slots.end(), slot);
        if (it == slots.end())
        {
            auto& s = slots.emplace_back(slot);
            ++uniqueIdCounter;
            s.setConnectionId(uniqueIdCounter);
            return uniqueIdCounter;
        }
        else
        {
            return it->getConnectionId();
        }
        return 0;
    }
    
    inline bool disconnect(const Slot& slot) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        const auto it = std::find(slots.begin(), slots.end(), slot);
        if (it != slots.end())
        {
            slots.erase(it);
            return true;
        }
        return false;
    }

};

} // namespace gusc::Threads
    
#endif /* Signal_hpp */
