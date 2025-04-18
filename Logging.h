
#pragma once
#include <Arduino.h>

class LoggingBase {
public:
    virtual ~LoggingBase() = default;

    // Core interface—pass by const& to avoid extra String copies.
    virtual void print(const String& msg) = 0;
    virtual void println(const String& msg) = 0;

    // Templated helpers for any printable type
    template <typename T>
    void print(const T& value) {
        print(String(value));
    }
    template <typename T>
    void println(const T& value) {
        println(String(value));
    }
};