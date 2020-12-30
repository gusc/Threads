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
        void operator()()
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
        Slot(Thread* initHostThread, const std::function<void(TArg...)>& initCallback)
            : hostThread(initHostThread)
            , callback(initCallback)
        {}
        
        bool operator==(const Slot& other)
        {
            typedef void(cbType)(TArg...);
            cbType* const* cbPtr = callback.template target<cbType*>();
            cbType* const* otherPtr = other.callback.template target<cbType*>();
            if (hostThread != other.hostThread)
            {
                return false;
            }
            if (cbPtr && otherPtr)
            {
                return *cbPtr == *otherPtr;
            }
            return false;
        }
        
        void call(const TArg&... args) const
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
        Thread* hostThread;
        std::function<void(TArg...)> callback;
        
        template<typename T, typename... U>
        size_t getFnAddress(std::function<T(U...)> f)
        {
            typedef T(fnType)(U...);
            fnType ** fnPointer = f.template target<fnType*>();
            return (size_t) *fnPointer;
        }
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
    /// @return false if listener has been already connected
    inline bool connect(Thread* thread, const std::function<void(TArg...)>& callback) noexcept
    {
        return connect({thread, callback});
    }
    
    /// @brief connect a listener callback to this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener has been already connected
    template<typename TClass>
    inline bool connect(TClass* thread, void(TClass::* callback)(const TArg&...)) noexcept
    {
        // TODO: figure out whether passing lambda to Slot does not break the whole unique connection logic (Slot comparisson)
        return connect(Slot{thread, [thread, callback](const TArg&... args){(thread->*callback)(args...);}});
    }

    /// @brief connect a listener callback to this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener from has been already connected
    template<typename TClass>
    inline bool connect(TClass* thread, void(TClass::* callback)(TArg...)) noexcept
    {
        return connect(Slot{thread, [thread, callback](const TArg&... args){(thread->*callback)(args...);}});
    }
    
    /// @brief disconnect a listener callback from this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    inline bool disconnect(Thread* thread, const std::function<void(TArg...)>& callback) noexcept
    {
        return disconnect({thread, callback});
    }
    
    /// @brief disconnect a listener callback from this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    template<typename TClass>
    inline bool disconnect(TClass* thread, void(TClass::* callback)(const TArg&...)) noexcept
    {
        return disconnect(Slot{thread, [thread, callback](const TArg&... args){(thread->*callback)(args...);}});
    }

    /// @brief disconnect a listener callback from this signal
    /// @param thread - listener's thread of affinity
    /// @param callback - listener's callback that will be called when signal is emitted
    /// @return false if listener was not connected
    template<typename TClass>
    inline bool disconnect(TClass* thread, void(TClass::* callback)(TArg...)) noexcept
    {
        return disconnect(Slot{thread, [thread, callback](const TArg&... args){(thread->*callback)(args...);}});
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
    std::mutex emitMutex;
    
    inline bool connect(const Slot& slot) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        if (std::find(slots.begin(), slots.end(), slot) == slots.end())
        {
            slots.emplace_back(slot);
            return true;
        }
        return false;
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

/// @brief template specialization for void arguments - unfortunatelly vararg templates can't handle that
template<>
class Signal<void>
{
    class Slot
    {
    public:
        Slot() = delete;
        Slot(Thread* initHostThread, const std::function<void(void)>& initCallback)
            : hostThread(initHostThread)
            , callback(initCallback)
        {}
        
        bool operator==(const Slot& other)
        {
            typedef void(cbType)(void);
            cbType* const* cbPtr = callback.template target<cbType*>();
            cbType* const* otherPtr = other.callback.template target<cbType*>();
            if (cbPtr && otherPtr)
            {
                return *cbPtr == *otherPtr;
            }
            return false;
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
        Thread* hostThread;
        std::function<void(void)> callback;
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
        return connect({thread, callback});
    }
    
    template<typename TClass>
    inline bool connect(TClass* thread, void(TClass::* callback)(void)) noexcept
    {
        return connect(Slot{thread, [thread, callback](){(thread->*callback)();}});
    }
    
    inline bool disconnect(Thread* thread, const std::function<void(void)>& callback) noexcept
    {
        return disconnect({thread, callback});
    }
    
    template<typename TClass>
    inline bool disconnect(TClass* thread, void(TClass::* callback)(void)) noexcept
    {
        return disconnect(Slot{thread, [thread, callback](){(thread->*callback)();}});
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
    std::mutex emitMutex;
    
    inline bool connect(const Slot& slot) noexcept
    {
        std::lock_guard<std::mutex> lock(emitMutex);
        if (std::find(slots.begin(), slots.end(), slot) == slots.end())
        {
            slots.emplace_back(slot);
            return true;
        }
        return false;
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

}
    
#endif /* Signal_hpp */
