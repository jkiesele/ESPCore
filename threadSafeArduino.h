#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * Thread-safe wrappers for common Arduino/ESP32 I/O.
 *
 * Usage:
 *   #include "thread_safe_arduino.hpp"
 *   void setup() {
 *     threadSafe::init();
 *     threadSafe::pinMode(2, OUTPUT);
 *     threadSafe::digitalWrite(2, HIGH);
 *     uint16_t t = threadSafe::touchRead(T7);
 *   }
 *
 * Notes:
 *  - Touch sensing uses a single global mutex (shared peripheral).
 *  - GPIO uses per-pin mutexes by default. Define THREADSAFE_GPIO_GLOBAL_LOCK
 *    before including this header to use one global GPIO mutex instead.
 *  - `analogRead()` is serialized here too; ADC2 can still contend with Wi‑Fi.
 *  - Do NOT call these from ISRs (locks are not ISR-safe).
 */

// --- Configuration -----------------------------------------------------------
#ifndef THREADSAFE_MAX_GPIO_PINS
  // Cover ESP32, ESP32-S2/S3; harmless if a bit large.
  #define THREADSAFE_MAX_GPIO_PINS  48
#endif

namespace threadSafe {

// ---- internals --------------------------------------------------------------
namespace detail {

  inline SemaphoreHandle_t createMutex() {
    auto m = xSemaphoreCreateMutex();
    // In embedded builds we usually assert; adapt if you prefer fail-soft.
    configASSERT(m != nullptr);
    return m;
  }

  inline bool inIsr() {
    return xPortInIsrContext() != 0;
  }

  // Global "table lock" used only to safely allocate per-pin locks
  inline SemaphoreHandle_t& tableLock() {
    static SemaphoreHandle_t m = createMutex();
    return m;
  }

  // Touch peripheral lock (single peripheral)
  inline SemaphoreHandle_t& touchLock() {
    static SemaphoreHandle_t m = createMutex();
    return m;
  }

  // Analog lock (ADC shared config/sequencer)
  inline SemaphoreHandle_t& analogLock() {
    static SemaphoreHandle_t m = createMutex();
    return m;
  }

#ifndef THREADSAFE_GPIO_GLOBAL_LOCK
  // Per-pin locks
  inline SemaphoreHandle_t* gpioLocks() {
    static SemaphoreHandle_t locks[THREADSAFE_MAX_GPIO_PINS] = { nullptr };
    return locks;
  }

  inline SemaphoreHandle_t getGpioLock(uint8_t pin) {
    if (pin >= THREADSAFE_MAX_GPIO_PINS) {
      // Out-of-range pin: fall back to a single global lock to be safe
      static SemaphoreHandle_t fallback = createMutex();
      return fallback;
    }
    auto* locks = gpioLocks();
    if (locks[pin] == nullptr) {
      // Lazily create under table lock
      xSemaphoreTake(tableLock(), portMAX_DELAY);
      if (locks[pin] == nullptr) {
        locks[pin] = createMutex();
      }
      xSemaphoreGive(tableLock());
    }
    return locks[pin];
  }
#else
  inline SemaphoreHandle_t& gpioGlobalLock() {
    static SemaphoreHandle_t m = createMutex();
    return m;
  }
  inline SemaphoreHandle_t getGpioLock(uint8_t /*pin*/) {
    return gpioGlobalLock();
  }
#endif

  // Small RAII guard
  struct LockGuard {
    SemaphoreHandle_t m{nullptr};
    explicit LockGuard(SemaphoreHandle_t mutex) : m(mutex) {
      // Skip locking in ISR context (illegal). Caller should not use from ISR.
      if (!inIsr()) xSemaphoreTake(m, portMAX_DELAY);
    }
    ~LockGuard() {
      if (!inIsr()) xSemaphoreGive(m);
    }
  };

} // namespace detail

// ---- public API -------------------------------------------------------------

inline void init() {
  // Touch/analog/global constructs initialize lazily on first use.
  // Creating tableLock here ensures FreeRTOS is up and avoids first-use races.
  (void)detail::tableLock();
}

// GPIO
inline void pinMode(uint8_t pin, uint8_t mode) {
  detail::LockGuard g(detail::getGpioLock(pin));
  ::pinMode(pin, mode);
}

inline void digitalWrite(uint8_t pin, uint8_t val) {
  detail::LockGuard g(detail::getGpioLock(pin));
  ::digitalWrite(pin, val);
}

inline int digitalRead(uint8_t pin) {
  detail::LockGuard g(detail::getGpioLock(pin));
  return ::digitalRead(pin);
}

// If you use shiftOut/shiftIn heavily, consider adding locked wrappers too.

// Analog (ADC1/ADC2)
inline int analogRead(uint8_t pin) {
  detail::LockGuard g(detail::analogLock());
  // NOTE: On classic ESP32, ADC2 conflicts with Wi‑Fi. This lock serializes
  //       *between your tasks*, but cannot resolve Wi‑Fi/ADC2 contention.
  //       Prefer ADC1 channels when Wi‑Fi is on.
  return ::analogRead(pin);
}

// Touch
inline uint16_t touchRead(uint8_t touchPin) {
  detail::LockGuard g(detail::touchLock());
  return ::touchRead(touchPin);
}

// Optional: expose try-locking variants for time-critical sections
inline bool digitalWriteTry(uint8_t pin, uint8_t val, TickType_t ticks_to_wait) {
  if (detail::inIsr()) return false;
  auto m = detail::getGpioLock(pin);
  if (xSemaphoreTake(m, ticks_to_wait) != pdTRUE) return false;
  ::digitalWrite(pin, val);
  xSemaphoreGive(m);
  return true;
}

} // namespace threadSafe