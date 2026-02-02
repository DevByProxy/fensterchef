#include "Application.hpp"
#include "../utils/Logger.hpp"
#include "../x11/X11Connection.hpp"
#include "../window/WindowManager.hpp"
#include <iostream>
#include <csignal>

namespace fensterchef {

// Global pointer for signal handling
static Application* g_application = nullptr;

static void signalHandler(int signal) {
    if (g_application) {
        LOG_INFO("Received signal, shutting down...");
        g_application->quit(0);
    }
}

Application::Application(int argc, char** argv)
    : x11_connection_(nullptr)
    , window_manager_(nullptr)
    , is_running_(false)
    , exit_code_(0)
{
    g_application = this;
    
    // Parse command line arguments
    parseArguments(argc, argv);
}

Application::~Application() {
    g_application = nullptr;
}

bool Application::parseArguments(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            std::cout << "Fensterchef - Modern C++ Tiling Window Manager\n";
            std::cout << "Usage: fensterchef [OPTIONS]\n";
            std::cout << "Options:\n";
            std::cout << "  -h, --help     Show this help\n";
            std::cout << "  -v, --version  Show version\n";
            std::cout << "  --verbose      Enable verbose logging\n";
            exit(0);
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Fensterchef 1.0.0\n";
            exit(0);
        } else if (arg == "--verbose") {
            Logger::instance().setLevel(Logger::Level::Debug);
        }
    }
    
    return true;
}

bool Application::initialize() {
    LOG_INFO("Initializing Fensterchef...");
    
    // Initialize logger
    Logger::instance().initialize("", Logger::Level::Info);
    
    // Initialize signal handlers
    if (!initializeSignalHandlers()) {
        return false;
    }
    
    // Initialize X11 connection
    if (!initializeX11()) {
        return false;
    }
    
    // Set global connection for windows (temporary hack)
    extern xcb_connection_t* g_connection;
    g_connection = x11_connection_->get();
    
    // Create window manager
    window_manager_ = std::make_unique<WindowManager>();
    
    LOG_INFO("Fensterchef initialized successfully");
    return true;
}

bool Application::initializeX11() {
    try {
        x11_connection_ = std::make_unique<X11Connection>();
        
        if (!x11_connection_->isValid()) {
            LOG_ERROR("Failed to connect to X11 server");
            return false;
        }
        
        // Take control as window manager
        return takeControl();
        
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("X11 initialization failed: ") + e.what());
        return false;
    }
}

bool Application::takeControl() {
    // Try to become the window manager by selecting SubstructureRedirect on root
    xcb_screen_t* screen = x11_connection_->getScreen();
    
    uint32_t values[] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
        XCB_EVENT_MASK_PROPERTY_CHANGE |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE
    };
    
    xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(
        x11_connection_->get(),
        screen->root,
        XCB_CW_EVENT_MASK,
        values
    );
    
    xcb_generic_error_t* error = xcb_request_check(x11_connection_->get(), cookie);
    
    if (error) {
        LOG_ERROR("Another window manager is already running");
        free(error);
        return false;
    }
    
    x11_connection_->flush();
    LOG_INFO("Successfully took control as window manager");
    return true;
}

bool Application::initializeSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    return true;
}

int Application::run() {
    LOG_INFO("Starting main event loop");
    is_running_ = true;
    
    while (is_running_) {
        xcb_generic_event_t* event = x11_connection_->waitForEvent();
        
        if (!event) {
            if (x11_connection_->hasError()) {
                LOG_ERROR("X11 connection error");
                break;
            }
            continue;
        }
        
        // Process event
        handleEvent(event);
        
        free(event);
        
        // Flush any pending requests
        x11_connection_->flush();
    }
    
    LOG_INFO("Event loop terminated");
    return exit_code_;
}

void Application::handleEvent(xcb_generic_event_t* event) {
    uint8_t response_type = event->response_type & ~0x80;
    
    switch (response_type) {
        case XCB_MAP_REQUEST: {
            auto* e = reinterpret_cast<xcb_map_request_event_t*>(event);
            handleMapRequest(e);
            break;
        }
        case XCB_UNMAP_NOTIFY: {
            auto* e = reinterpret_cast<xcb_unmap_notify_event_t*>(event);
            handleUnmapNotify(e);
            break;
        }
        case XCB_DESTROY_NOTIFY: {
            auto* e = reinterpret_cast<xcb_destroy_notify_event_t*>(event);
            handleDestroyNotify(e);
            break;
        }
        case XCB_CONFIGURE_REQUEST: {
            auto* e = reinterpret_cast<xcb_configure_request_event_t*>(event);
            handleConfigureRequest(e);
            break;
        }
        case XCB_KEY_PRESS: {
            auto* e = reinterpret_cast<xcb_key_press_event_t*>(event);
            handleKeyPress(e);
            break;
        }
        default:
            // Ignore unknown events for now
            break;
    }
}

void Application::handleMapRequest(xcb_map_request_event_t* event) {
    LOG_DEBUG("Map request for window");
    
    // Create and manage the window
    Window* window = window_manager_->createWindow(event->window);
    
    if (window) {
        // Map the window
        xcb_map_window(x11_connection_->get(), event->window);
    }
}

void Application::handleUnmapNotify(xcb_unmap_notify_event_t* event) {
    LOG_DEBUG("Unmap notify for window");
    
    Window* window = window_manager_->findWindow(event->window);
    if (window) {
        // Handle window unmapping
        // For now, just log it
    }
}

void Application::handleDestroyNotify(xcb_destroy_notify_event_t* event) {
    LOG_DEBUG("Destroy notify for window");
    
    Window* window = window_manager_->findWindow(event->window);
    if (window) {
        window_manager_->destroyWindow(window);
    }
}

void Application::handleConfigureRequest(xcb_configure_request_event_t* event) {
    LOG_DEBUG("Configure request for window");
    
    // For now, just grant the request
    uint32_t values[7];
    int i = 0;
    
    if (event->value_mask & XCB_CONFIG_WINDOW_X) values[i++] = event->x;
    if (event->value_mask & XCB_CONFIG_WINDOW_Y) values[i++] = event->y;
    if (event->value_mask & XCB_CONFIG_WINDOW_WIDTH) values[i++] = event->width;
    if (event->value_mask & XCB_CONFIG_WINDOW_HEIGHT) values[i++] = event->height;
    if (event->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) values[i++] = event->border_width;
    if (event->value_mask & XCB_CONFIG_WINDOW_SIBLING) values[i++] = event->sibling;
    if (event->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) values[i++] = event->stack_mode;
    
    xcb_configure_window(x11_connection_->get(), event->window, event->value_mask, values);
}

void Application::handleKeyPress(xcb_key_press_event_t* event) {
    // For now, just log key presses
    LOG_DEBUG("Key press event");
    
    // If it's Mod+Q, quit the application (simple test binding)
    if (event->state & XCB_MOD_MASK_4) { // Mod4 is usually Super
        // Could check specific key here
    }
}

void Application::quit(int exit_code) {
    exit_code_ = exit_code;
    is_running_ = false;
}

} // namespace fensterchef
