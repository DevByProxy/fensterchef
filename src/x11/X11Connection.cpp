#include "X11Connection.hpp"
#include "../utils/Logger.hpp"
#include <stdexcept>

namespace fensterchef {

X11Connection::X11Connection(const char* display_name)
    : connection_(nullptr)
    , screen_(nullptr)
    , screen_number_(0)
{
    connection_ = xcb_connect(display_name, &screen_number_);
    
    if (hasError()) {
        throw std::runtime_error("Failed to connect to X server");
    }
    
    // Get the screen
    const xcb_setup_t* setup = xcb_get_setup(connection_);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    
    for (int i = 0; i < screen_number_; ++i) {
        xcb_screen_next(&iter);
    }
    
    screen_ = iter.data;
    
    if (!screen_) {
        xcb_disconnect(connection_);
        connection_ = nullptr;
        throw std::runtime_error("Failed to get X screen");
    }
    
    LOG_INFO("Connected to X11 server");
}

X11Connection::~X11Connection() {
    if (connection_) {
        xcb_disconnect(connection_);
        LOG_INFO("Disconnected from X11 server");
    }
}

bool X11Connection::hasError() const noexcept {
    if (!connection_) {
        return true;
    }
    return xcb_connection_has_error(connection_) != 0;
}

void X11Connection::flush() {
    if (connection_) {
        xcb_flush(connection_);
    }
}

xcb_generic_event_t* X11Connection::waitForEvent() {
    if (!connection_) {
        return nullptr;
    }
    return xcb_wait_for_event(connection_);
}

xcb_generic_event_t* X11Connection::pollForEvent() {
    if (!connection_) {
        return nullptr;
    }
    return xcb_poll_for_event(connection_);
}

uint32_t X11Connection::generateId() {
    if (!connection_) {
        return 0;
    }
    return xcb_generate_id(connection_);
}

} // namespace fensterchef
