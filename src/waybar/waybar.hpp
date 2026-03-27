#pragma once
#include <string>

namespace waybar {

void set_idle();
void set_listening();
void set_transcript(const std::string& spoken);
void set_action(const std::string& action_name);

} // namespace waybar