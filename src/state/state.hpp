#pragma once

namespace state {

enum class State { Idle, Listening };

void  set(State s);
State get();
void  reset_timeout();
bool  is_timed_out();
bool  is_idle_timed_out(int seconds);

} // namespace state