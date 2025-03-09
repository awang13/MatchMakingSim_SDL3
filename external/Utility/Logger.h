#pragma once
#include <chrono>
#include <iostream>
#include <sstream>

struct FScopedPerfTimer
{
    const char* Name;
    std::chrono::high_resolution_clock::time_point StartTime;

    explicit FScopedPerfTimer(const char* InName)
        : Name(InName), StartTime(std::chrono::high_resolution_clock::now()) {}

    ~FScopedPerfTimer()
    {
        auto EndTime = std::chrono::high_resolution_clock::now();
        double ElapsedTime = std::chrono::duration<double, std::milli>(EndTime - StartTime).count();
        std::cout << "PERF TIMER: " << Name << " execution Time: " << ElapsedTime << "ms\n";
    }
};

struct FSimpleLogger
{
    const char* Name;
    std::string LogContext;

    explicit FSimpleLogger(const char* inName, const std::string& inContext)
        : Name(inName), LogContext(inContext) {}

    ~FSimpleLogger()
    {
        std::cout << "LOG " << Name << ": " << LogContext << "\n";
    }
};

// Macros to simplify usage
#define START_PERF_MEASURE(NAME) FScopedPerfTimer PerfTimer_##NAME(#NAME);
#define SIMPLOG(NAME, TEXT) FSimpleLogger Logger_##NAME(#NAME, ToString(TEXT));

// Generic conversion function to handle multiple types
template <typename T>
std::string ToString(const T& value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}