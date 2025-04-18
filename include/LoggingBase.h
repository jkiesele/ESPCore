#ifndef LOGGING_BASE_H
#define LOGGING_BASE_H

#include <Arduino.h>

class LoggingBase {
public:
    virtual ~LoggingBase() = default;

    // Core interface—pass by const& to avoid extra String copies.
    virtual void print(const String& msg) = 0;
    virtual void println(const String& msg) = 0;

    virtual void print(const char* msg) { print(String(msg)); }
    virtual void println(const char* msg) { println(String(msg)); }

    virtual void print(const __FlashStringHelper* msg) { print(String(msg)); }
    virtual void println(const __FlashStringHelper* msg) { println(String(msg)); }

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


class SerialLogging : public LoggingBase {
public:
    void print(const String& msg) override {
        Serial.print(msg);
    }
    void println(const String& msg) override {
        Serial.println(msg);
    }
};

class NullLogging : public LoggingBase {
public:
    void print(const String& msg) override {
        // No operation
    }
    void println(const String& msg) override {
        // No operation
    }
};

extern LoggingBase* gLogger;

void setLogger(LoggingBase* logger);
#endif