#pragma once

namespace state {

enum class State { Idle, Listening };

void  set(State s);
State get();
void  reset_timeout();
bool  is_timed_out();

} // namespace state