//
//  Utilities.hpp
//  Threads
//
//  Created by Gusts Kaksis on 28/11/2020.
//  Copyright © 2020 Gusts Kaksis. All rights reserved.
//

#ifndef Utilities_h
#define Utilities_h

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <vector>
#include <thread>

inline std::string tidToStr(const std::thread::id& id)
{
    std::ostringstream ss;
    ss << id;
    return ss.str();
}

class Logger
{
public:
    Logger& operator<<(const std::string& row)
    {
        std::lock_guard<std::mutex> lock(mutex);
        rows.push_back(row);
        return *this;
    }
    ~Logger()
    {
        print();
    }
    inline void print() noexcept
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& s : rows)
        {
            std::cout << s << std::endl;
        }
        rows.clear();
    }
    
private:
    std::vector<std::string> rows;
    std::mutex mutex;
};

#endif /* Utilities_h */
