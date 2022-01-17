//
//  Signal.hpp
//  Threads
//
//  Created by Gusts Kaksis on 21/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#ifndef Signal_hpp
#define Signal_hpp

#include "Thread.hpp"

#include <tuple>
#include <map>

namespace gusc::Threads
{

/// @brief class representing a signal connection and emission object
template<typename ...TArg>
class Signal
{
    /// @brief internal class representing a single message that wraps the signal data and the listener and is dispatched to a listener's thread
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
    
    /// @brief internal class representing a signal connection slot (listener and it's affinity thread)
    class Slot
    {
    public:
        Slot() = delete;
        Slot(Thread* initHostThread, void* initCallbackPtr, const std::function<void(TArg...)>& initCallback)
            : hostThread(initHostThread)
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
            return callbackPtr && hostThread == other.hostThread && callbackPtr != other.callbackPtr;
        }
        
        inline void call(const TArg&... args) const
        {
            if (!hostThread)
            {
                throw std::runtime_error("Host thread is null");
            }
            if (*hostThread == std::this_thread::get_id())
            {
                callback(args...);
            }
            else
            {
                hostThread->send(SignalMessage{callback, args...});
            }
        }
        
    private:
        Thread* hostThread { nullptr };
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
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return connection ID for disconnecting the slot later or 0 if failed to insert the slot
    inline size_t connect(Thread* thread, const std::function<void(const TArg&...)>& callback) noexcept
    {
        typedef void(fnType)(const TArg&...);
        fnType* const* fnPointer = callback.template target<fnType*>();
        return connect({thread, fnPointer ? reinterpret_cast<void*>(*fnPointer) : nullptr, callback});
    }

    /// @brief connect a listener callback to this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return connection ID for disconnecting the slot later or 0 if failed to insert the slot
    template<typename TClass>
    inline size_t connect(TClass* thread, void(TClass::* callback)(const TArg&...)) noexcept
    {
        return connect(Slot{thread, reinterpret_cast<void*&>(callback), [thread, callback](const TArg&... args){(thread->*callback)(args...);}});
    }

    /// @brief connect a listener callback to this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return connection ID for disconnecting the slot later or 0 if failed to insert the slot
    template<typename TClass>
    inline size_t connect(TClass* thread, void(TClass::* callback)(TArg...)) noexcept
    {
        return connect(Slot{thread, reinterpret_cast<void*&>(callback), [thread, callback](const TArg&... args){(thread->*callback)(args...);}});
    }
    
    /// @brief disconnect a listener callback from this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    inline bool disconnect(Thread* thread, const std::function<void(const TArg&...)>& callback) noexcept
    {
        typedef void(fnType)(const TArg&...);
        fnType* const* fnPointer = callback.template target<fnType*>();
        return disconnect({thread, fnPointer ? reinterpret_cast<void*>(*fnPointer) : nullptr, callback});
    }
    
    /// @brief disconnect a listener callback from this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    template<typename TClass>
    inline bool disconnect(TClass* thread, void(TClass::* callback)(const TArg&...)) noexcept
    {
        return disconnect(Slot{thread, reinterpret_cast<void*&>(callback), [thread, callback](const TArg&... args){(thread->*callback)(args...);}});
    }

    /// @brief disconnect a listener callback from this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    template<typename TClass>
    inline bool disconnect(TClass* thread, void(TClass::* callback)(TArg...)) noexcept
    {
        return disconnect(Slot{thread, reinterpret_cast<void*&>(callback), [thread, callback](const TArg&... args){(thread->*callback)(args...);}});
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
        Slot(Thread* initHostThread, void* initCallbackPtr, const std::function<void(void)>& initCallback)
            : hostThread(initHostThread)
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
            return callbackPtr && hostThread == other.hostThread && callbackPtr == other.callbackPtr;
        }
        
        void call() const
        {
            if (*hostThread == std::this_thread::get_id())
            {
                callback();
            }
            else
            {
                hostThread->send(callback);
            }
        }
        
    private:
        Thread* hostThread { nullptr };
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
    
    inline bool connect(Thread* thread, const std::function<void(void)>& callback) noexcept
    {
        typedef void(fnType)(void);
        fnType* const* fnPointer = callback.target<fnType*>();
        return connect({thread, fnPointer ? reinterpret_cast<void*>(*fnPointer) : nullptr, callback});
    }
    
    template<typename TClass>
    inline bool connect(TClass* thread, void(TClass::* callback)(void)) noexcept
    {
        return connect(Slot{thread, reinterpret_cast<void*&>(callback), [thread, callback](){(thread->*callback)();}});
    }
    
    inline bool disconnect(Thread* thread, const std::function<void(void)>& callback) noexcept
    {
        typedef void(fnType)(void);
        fnType* const* fnPointer = callback.target<fnType*>();
        return disconnect({thread, fnPointer ? reinterpret_cast<void*>(*fnPointer) : nullptr, callback});
    }
    
    template<typename TClass>
    inline bool disconnect(TClass* thread, void(TClass::* callback)(void)) noexcept
    {
        return disconnect(Slot{thread, reinterpret_cast<void*&>(callback), [thread, callback](){(thread->*callback)();}});
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
            const auto slotIndex = std::distance(slots.begin(), it);
            slots.erase(it);
            return true;
        }
        return false;
    }

};

}
    
#endif /* Signal_hpp */
