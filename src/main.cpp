/**
 * @file main.cpp
 * @brief Entry point for Fensterchef window manager
 */

#include "core/Application.hpp"
#include "utils/Logger.hpp"
#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        // Create application instance
        fensterchef::Application app(argc, argv);
        
        // Initialize all subsystems
        if (!app.initialize()) {
            LOG_ERROR("Failed to initialize application");
            return EXIT_FAILURE;
        }
        
        // Run the main event loop
        return app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        LOG_ERROR(std::string("Fatal exception: ") + e.what());
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        LOG_ERROR("Unknown fatal exception");
        return EXIT_FAILURE;
    }
}
