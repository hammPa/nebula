#include "state.hpp"
#include "../config/config.hpp"
#include <chrono>

namespace state {

static State s_current = State::Idle;
static std::chrono::steady_clock::time_point s_last_active;

void set(State s)    { s_current = s; }
State get()          { return s_current; }
void reset_timeout() { s_last_active = std::chrono::steady_clock::now(); }

bool is_timed_out() {
    if (s_current != State::Listening) return false;
    auto elapsed = std::chrono::steady_clock::now() - s_last_active;
    return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
           >= g_listenTimeoutSec;
}

} // namespace state