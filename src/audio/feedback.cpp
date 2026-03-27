#include "feedback.hpp"
#include "../utils/shell.hpp"

namespace audio {

static constexpr auto SOUND_LOGIN   = "paplay /usr/share/sounds/freedesktop/stereo/service-login.oga 2>/dev/null";
static constexpr auto SOUND_LOGOUT  = "paplay /usr/share/sounds/freedesktop/stereo/service-logout.oga 2>/dev/null";
static constexpr auto SOUND_MESSAGE = "paplay /usr/share/sounds/freedesktop/stereo/message.oga 2>/dev/null";

void feedback(Event ev) {
    switch (ev) {
        case Event::Wake:
            shell::run_bg("notify-send 'Nebula' '🎙️ Mendengarkan...' -t 800 -i audio-input-microphone");
            shell::run_bg(SOUND_LOGIN);
            break;
        case Event::Action:
            shell::run_bg(SOUND_MESSAGE);
            break;
        case Event::Idle:
            shell::run_bg("notify-send 'Nebula' '💤 Standby' -t 600");
            shell::run_bg(SOUND_LOGOUT);
            break;
        case Event::Timeout:
            shell::run_bg("notify-send 'Nebula' '⏱️ Timeout — kembali standby' -t 800");
            shell::run_bg(SOUND_LOGOUT);
            break;
        case Event::MicError:
            shell::run_bg("notify-send 'Nebula' '🎤 Mic error' -t 1500");
            break;
    }
}

} // namespace audio