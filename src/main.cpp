#include "core/Engine.h"
#include "core/Logger.h"
#include <cstdlib>

int main() {
    // Set up logging
    ps::Logger::instance().setLevel(ps::LogLevel::INFO);
    ps::Logger::instance().setLogFile("phantom_strike.log");

    LOG_INFO("Phantom Strike v0.4.0 starting...");

    // Configure window
    ps::Window::Config config;
    config.width = 1280;
    config.height = 720;
    config.title = "Phantom Strike - v0.4.0 [Phase 4: FPS Mechanics]";
    config.vsync = true;
    config.msaaSamples = 4;

    // Create and run engine
    ps::Engine engine;
    if (!engine.init(config)) {
        LOG_FATAL("Engine initialization failed!");
        return EXIT_FAILURE;
    }

    engine.run();

    LOG_INFO("Phantom Strike shutdown cleanly.");
    return EXIT_SUCCESS;
}
