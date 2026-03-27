#pragma once

namespace audio {

enum class Event { Wake, Action, Idle, Timeout, MicError };

void feedback(Event ev);

} // namespace audio