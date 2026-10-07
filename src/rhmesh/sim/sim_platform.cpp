// Host simulator only. Boards compile this file empty.
#ifndef ARDUINO

// Host versions of the Arduino functions RadioHead declares in RHutil/simulator.h.
#include <RadioHead.h>

#include <chrono>
#include <random>
#include <thread>

int _simulator_argc = 0;
char** _simulator_argv = nullptr;
SerialSimulator Serial;

static const auto kStart = std::chrono::steady_clock::now();

unsigned long millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - kStart)
      .count();
}

void delay(unsigned long ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

// One generator per thread, so nodes don't contend on a lock.
static std::mt19937& rng() {
  thread_local std::mt19937 gen{std::random_device{}()};
  return gen;
}

long random(long to) { return to <= 0 ? 0 : std::uniform_int_distribution<long>(0, to - 1)(rng()); }

long random(long from, long to) { return to <= from ? from : from + random(to - from); }

#endif  // ARDUINO
