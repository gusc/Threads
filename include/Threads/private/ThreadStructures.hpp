//
//  ThreadProcedure.hpp
//  Threads
//
//  Created by Gusts Kaksis on 25/10/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//


class CallableContainerBase
{
public:
    virtual ~CallableContainerBase() = default;
    virtual void operator()(const Thread::StopToken&) const = 0;
};

template<class TFn, class ...TArgs>
class CallableContainer final : public CallableContainerBase
{
public:
    explicit CallableContainer(TFn&& initFn, TArgs&&... initArgs)
        : fn(std::forward<TFn>(initFn))
        , args(std::make_tuple<TArgs&&...>(std::forward<TArgs>(initArgs)...))
    {}
    
    void operator()(const Thread::StopToken& stopToken) const override
    {
        callSpec<TFn>(stopToken, std::index_sequence_for<TArgs...>());
    }
    
private:
    TFn fn;
    std::tuple<TArgs...> args;
    
    template<typename T, std::size_t... Is>
    inline
    typename std::enable_if_t<
        std::is_invocable<T, Thread::StopToken, TArgs...>::value,
        void> callSpec(const Thread::StopToken& stopToken, std::index_sequence<Is...>) const
    {
        std::invoke(fn, stopToken, std::get<Is>(args)...);
    }
    
    template<typename T, std::size_t... Is>
    inline
    typename std::enable_if_t<
        !std::is_invocable<T, Thread::StopToken, TArgs...>::value,
        void> callSpec(const Thread::StopToken&, std::index_sequence<Is...>) const
    {
        std::invoke(fn, std::get<Is>(args)...);
    }
};

class ThreadProcedure final
{
public:
    ThreadProcedure() = default;
    
    template<class TFn, class ...TArgs>
    explicit ThreadProcedure(TFn&& fn, TArgs&&... args)
        : callableObject(std::make_unique<CallableContainer<TFn, TArgs...>>(std::forward<TFn>(fn), std::forward<TArgs>(args)...))
    {}
    
    inline void operator()(const Thread::StopToken& stopToken) const
    {
        if (callableObject)
        {
            (*callableObject)(stopToken);
        }
    }
    
private:
    std::unique_ptr<CallableContainerBase> callableObject;
};
