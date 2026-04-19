#include "feedback.hpp"
#include "../utils/shell.hpp"

namespace audio {

static constexpr auto WAKE_CMD    = 
    "notify-send 'Nebula' '🎙️ Mendengarkan...' -t 800 -i audio-input-microphone"
    " & paplay /usr/share/sounds/freedesktop/stereo/service-login.oga 2>/dev/null";

static constexpr auto IDLE_CMD    = 
    "notify-send 'Nebula' '💤 Standby' -t 600"
    " & paplay /usr/share/sounds/freedesktop/stereo/service-logout.oga 2>/dev/null";

static constexpr auto TIMEOUT_CMD = 
    "notify-send 'Nebula' '⏱️ Timeout — kembali standby' -t 800"
    " & paplay /usr/share/sounds/freedesktop/stereo/service-logout.oga 2>/dev/null";

static constexpr auto SOUND_MESSAGE = 
    "paplay /usr/share/sounds/freedesktop/stereo/message.oga 2>/dev/null";

static constexpr auto NOTIFY_MIC_ERR = 
    "notify-send 'Nebula' '🎤 Mic error' -t 1500";

void feedback(Event ev) {
    switch (ev) {
        case Event::Wake:       shell::run_bg(WAKE_CMD);        break;
        case Event::Action:     shell::run_bg(SOUND_MESSAGE);   break;
        case Event::Idle:       shell::run_bg(IDLE_CMD);        break;
        case Event::Timeout:    shell::run_bg(TIMEOUT_CMD);     break;
        case Event::MicError:   shell::run_bg(NOTIFY_MIC_ERR);  break;
    }
}

} // namespace audio