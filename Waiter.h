#ifndef WAITER_H
#define WAITER_H

#include <chrono>
#include <thread>

class Waiter {
private:
  std::chrono::high_resolution_clock::time_point lasttime;
  std::chrono::milliseconds tick_length;

  Waiter(Waiter const &rhs) = delete;
  Waiter(Waiter &&rhs) = delete;
  Waiter &operator=(Waiter const &rhs) noexcept = delete;
  Waiter &operator=(Waiter &&rhs) noexcept = delete;

public:
  Waiter(unsigned int ms_per_tick);
  ~Waiter() noexcept {}
  void wait_if_fast();
  void set_ms_tick_length(unsigned int ms_per_tick);
};

#endif /* end of include guard: WAITER_H */
