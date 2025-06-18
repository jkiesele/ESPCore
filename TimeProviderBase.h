#ifndef TIME_PROVIDER_BASE_H
#define TIME_PROVIDER_BASE_H

#include <Arduino.h>

class TimeProviderBase {
public:

    virtual ~TimeProviderBase() {}
    virtual void begin() = 0;
    
    virtual uint32_t getUnixTime()  = 0;
    virtual uint32_t getUnixUTCTime(uint32_t localTime=0)=0;
    virtual String getFormattedTime() = 0;

    virtual int getSecondsOfDay() = 0;
};

//just uses millis()
class NullTimeProvider : public TimeProviderBase {
public:
    void begin() override {}
    
    uint32_t getUnixTime() override {
        return millis();
    }
    
    uint32_t getUnixUTCTime(uint32_t localTime=0) override {
        return localTime;
    }
    String getFormattedTime() override {
        return String(millis()/1000);
    }

    int getSecondsOfDay() override { //wraps at 24h
        int secs = millis() / 1000;
        return secs % 86400; // 24 * 60 * 60
    }

};

extern TimeProviderBase* gTimeProvider;
extern NullTimeProvider gNullTimeProvider;

void setTimeProvider(TimeProviderBase* timeProvider);

#endif