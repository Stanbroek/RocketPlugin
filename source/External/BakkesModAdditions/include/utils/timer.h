#pragma once
#include <string>
#include <chrono>

#include "fmt/chrono.h"


class Timer
{
public:
    Timer() : timePoint(std::chrono::system_clock::now()) {}
    ~Timer() = default;

    std::chrono::system_clock::duration Duration() const
    {
        return std::chrono::system_clock::now() - timePoint;
    }

    std::string Str() const
    {
        const std::chrono::system_clock::duration duration = Duration();

        if (auto hours = std::chrono::duration_cast<std::chrono::hours>(duration); hours.count() > 0) {
            return fmt::format("{}", hours);
        }
        if (auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration); minutes.count() > 0) {
            return fmt::format("{}", minutes);
        }
        if (auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration); seconds.count() > 0) {
            return fmt::format("{}", seconds);
        }
        if (auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration); milliseconds.count() > 0) {
            return fmt::format("{}", milliseconds);
        }
        if (auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration); microseconds.count() > 0) {
            return fmt::format("{}", microseconds);
        }
        if (auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration); nanoseconds.count() > 0) {
            return fmt::format("{}", nanoseconds);
        }

        return fmt::format("{}", duration);
    }

    Timer(Timer&&) = delete;
    Timer(const Timer&) = delete;
    Timer& operator=(Timer&&) = delete;
    Timer& operator=(const Timer&) = delete;

private:
    std::chrono::system_clock::time_point timePoint;
};
