#include <csignal>
#include <malloc.h>
#include "engine/nebula_engine.hpp"
#include "utils/logger.hpp"

volatile std::sig_atomic_t g_running = 1;
static void signal_handler(int) { g_running = 0; }

int main(int argc, char* argv[]) {
    // Optimasi Memori
    mallopt(M_TRIM_THRESHOLD, 128 * 1024);
    mallopt(M_MMAP_THRESHOLD, 64 * 1024);

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    logger::banner();

    NebulaEngine engine;
    const std::string config_path = (argc > 1) ? argv[1] : "commands.json";

    if (engine.init(config_path)) {
        engine.run();
    }

    engine.cleanup();
    return 0;
}