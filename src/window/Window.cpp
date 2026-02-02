#include "Window.hpp"
#include "../utils/Logger.hpp"
#include <cstring>

namespace fensterchef {

// External X11 connection (will be passed in properly later)
extern xcb_connection_t* g_connection;

Window::Window(xcb_window_t xcb_window)
    : xcb_id_(xcb_window)
    , number_(0)
    , x_(0)
    , y_(0)
    , width_(0)
    , height_(0)
    , border_size_(1)
    , border_color_(0x0000FF)  // Blue border
    , mode_(Mode::Floating)
    , previous_mode_(Mode::Floating)
    , is_visible_(false)
    , is_mapped_(false)
    , close_requested_(false)
    , close_request_time_(0)
    , frame_(nullptr)
    , above_(nullptr)
    , below_(nullptr)
    , next_(nullptr)
    , newer_(nullptr)
{
    std::memset(&size_hints_, 0, sizeof(size_hints_));
    std::memset(&wm_hints_, 0, sizeof(wm_hints_));
    std::memset(&floating_geometry_, 0, sizeof(floating_geometry_));
    
    LOG_DEBUG("Window created");
}

Window::~Window() {
    LOG_DEBUG("Window destroyed");
}

void Window::setPosition(int32_t x, int32_t y) noexcept {
    x_ = x;
    y_ = y;
}

void Window::setSize(uint32_t width, uint32_t height) noexcept {
    width_ = width;
    height_ = height;
}

void Window::setGeometry(int32_t x, int32_t y, uint32_t width, uint32_t height) noexcept {
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
}

void Window::close(bool force) {
    LOG_DEBUG("Closing window");
    
    if (force || close_requested_) {
        // Force kill
        if (g_connection) {
            xcb_kill_client(g_connection, xcb_id_);
        }
    } else {
        // Try graceful close first
        // TODO: Send WM_DELETE_WINDOW message
        close_requested_ = true;
        close_request_time_ = std::time(nullptr);
    }
}

void Window::show() {
    LOG_DEBUG("Showing window");
    if (g_connection) {
        xcb_map_window(g_connection, xcb_id_);
        is_visible_ = true;
        is_mapped_ = true;
    }
}

void Window::hide() {
    LOG_DEBUG("Hiding window");
    if (g_connection) {
        xcb_unmap_window(g_connection, xcb_id_);
        is_visible_ = false;
        is_mapped_ = false;
    }
}

bool Window::acceptsFocus() const {
    // For now, all windows accept focus
    return mode_ != Mode::Dock;
}

void Window::focus() {
    LOG_DEBUG("Focusing window");
    if (g_connection) {
        xcb_set_input_focus(g_connection, XCB_INPUT_FOCUS_POINTER_ROOT, 
                           xcb_id_, XCB_CURRENT_TIME);
    }
}

void Window::getMinimumSize(Size& size) const {
    // TODO: Read from size hints
    size.width = 50;
    size.height = 50;
}

void Window::getMaximumSize(Size& size) const {
    // TODO: Read from size hints
    size.width = 10000;
    size.height = 10000;
}

void Window::ensureInBounds() {
    // TODO: Implement bounds checking
}

void Window::updateStackPosition() {
    // TODO: Implement Z-order management
}

} // namespace fensterchef
