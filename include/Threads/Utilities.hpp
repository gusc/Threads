//
//  Utilities.hpp
//  Threads
//
//  Created by Gusts Kaksis on 15/11/2022.
//  Copyright © 2022 Gusts Kaksis. All rights reserved.
//

#ifndef Utilities_hpp
#define Utilities_hpp

#include <mutex>

namespace gusc
{

template <typename T, typename M>
class LockedReference
{
public:
    LockedReference(T& initRef, M& initMutex)
        : ref(initRef)
        , lock(initMutex) {}
    LockedReference(const LockedReference&) = delete;
    LockedReference operator=(const LockedReference&) = delete;
    LockedReference(LockedReference&&) = delete;
    LockedReference operator=(LockedReference&&) = delete;
    ~LockedReference() = default;
    
    inline T* operator->() noexcept
    {
        return &ref;
    }
    
    inline T& operator*() noexcept
    {
        return ref;
    }
    
    inline const T* operator->() const noexcept
    {
        return &ref;
    }
    
    inline const T& operator*() const noexcept
    {
        return ref;
    }
    
private:
    T& ref;
    std::lock_guard<M> lock;
};
    
} // namespace gusc

#endif /* Utilities_hpp */
