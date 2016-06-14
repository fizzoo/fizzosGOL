#ifndef WAITER_H
#define WAITER_H

#include <chrono>
#include <thread>

/**
 * Class for synchronizing a thread to some hz.
 * Provides functionality to wait until the next tick, and deal with not being
 * fast enough.
 */
class Waiter {
private:
  std::chrono::high_resolution_clock::time_point lasttime;
  std::chrono::milliseconds tick_length;
  int nr_slow_runs = 0;

  Waiter(Waiter const &rhs) = delete;
  Waiter(Waiter &&rhs) = delete;
  Waiter &operator=(Waiter const &rhs) noexcept = delete;
  Waiter &operator=(Waiter &&rhs) noexcept = delete;

public:
  Waiter(unsigned int ms_per_tick);
  ~Waiter() noexcept {}

  /**
   * Sleeps this thread if there is time left on the tick, otherwise continues.
   */
  void wait_if_fast();

  /**
   * Sets the amount of milliseconds per tick.
   */
  void set_ms_tick_length(unsigned int ms_per_tick);

  /**
   * Number of consecutive slow runs (wait_if_fasts that didn't wait).
   */
  int slow_runs() const;
};

#endif /* end of include guard: WAITER_H */
