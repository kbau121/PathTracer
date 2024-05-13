#pragma once

#include <chrono>
#include <functional>

#define UNUSED(IDENTIFIER) (void)IDENTIFIER

class DeferGuard {
public:
    DeferGuard(std::function<void(void)> callback)
        : _callback(callback)
    {
    }

    ~DeferGuard()
    {
        _callback();
    }

    DeferGuard(const DeferGuard&) = delete;
    DeferGuard& operator=(const DeferGuard&) = delete;

private:
    std::function<void(void)> _callback;
};

#define DEFER_CONCAT(A, B) A##B
#define UNIQUE_DEFER_IDENTIFIER_FROM_LINE(L) DEFER_CONCAT(_deferGuard, L)
#define UNIQUE_DEFER_IDENTIFIER UNIQUE_DEFER_IDENTIFIER_FROM_LINE(__LINE__)

#define DEFER(EXPR)                          \
    DeferGuard UNIQUE_DEFER_IDENTIFIER([&] { \
        EXPR;                                \
    })

class Timer {
public:
    Timer()
        : _startTime(std::chrono::high_resolution_clock::now())
    {
    }

    ~Timer()
    {
    }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    double getElapsedSeconds() const
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = endTime - _startTime;
        return duration.count();
    }

    double getElapsedMilliseconds() const
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = endTime - _startTime;
        return duration.count() * 1000.0;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
};
