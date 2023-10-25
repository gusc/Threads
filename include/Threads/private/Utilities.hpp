//
//  Utilities.hpp
//  Threads
//
//  Created by Gusts Kaksis on 25/10/2023.
//  Copyright © 2023 Gusts Kaksis. All rights reserved.
//

#ifndef GUSC_UTILITIES_HPP
#define GUSC_UTILITIES_HPP

#include <mutex>

namespace gusc
{

template<class TA, class TB>
using IsSameType = std::is_same<
    typename std::decay<
        typename std::remove_cv<
            typename std::remove_reference<TA>::type
        >::type
    >::type,
    TB
>;    
} // namespace gusc

#endif /* GUSC_UTILITIES_HPP */
