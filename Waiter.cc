#include "Waiter.h"

Waiter::Waiter(unsigned int ms_per_tick)
    : lasttime(std::chrono::high_resolution_clock::now()),
      tick_length(ms_per_tick) {}

void Waiter::wait_if_fast() {
  auto goal_time = lasttime + tick_length;

  // lasttime refreshed for next tick
  lasttime = std::chrono::high_resolution_clock::now();
  if (goal_time > lasttime) {
    nr_slow_runs = 0;
    std::this_thread::sleep_until(goal_time);
  } else {
    ++nr_slow_runs;
  }
}

void Waiter::set_ms_tick_length(unsigned int ms_per_tick) {
  tick_length = std::chrono::milliseconds(ms_per_tick);
}

int Waiter::slow_runs() const {
  return nr_slow_runs;
}
