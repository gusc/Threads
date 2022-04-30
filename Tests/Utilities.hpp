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
#include <chrono>
#include <iomanip>

using namespace std::chrono_literals;

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
        auto time = getTimestamp();
        auto str = "[" + time + "] " + row;
        std::lock_guard<std::mutex> lock(mutex);
        rows.push_back(str);
        return *this;
    }
    ~Logger()
    {
        flush();
    }
    inline void flush() noexcept
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& s : rows)
        {
            std::cout << s << std::endl;
        }
        rows.clear();
    }
    static inline std::string getTimestamp() noexcept
    {
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now - 24h);
        const auto tm = *std::localtime(&time);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return oss.str() + " (" + std::to_string(timestamp) + ")";
    }
    
private:
    std::vector<std::string> rows;
    std::mutex mutex;
};

#endif /* Utilities_h */
