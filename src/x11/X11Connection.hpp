#ifndef FENSTERCHEF_X11_X11CONNECTION_HPP
#define FENSTERCHEF_X11_X11CONNECTION_HPP

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <memory>
#include <string>

namespace fensterchef {

/**
 * @brief Wraps XCB connection and provides RAII management
 */
class X11Connection {
public:
    /**
     * @brief Connect to X11 server
     * @param display_name Display name (nullptr for default)
     */
    explicit X11Connection(const char* display_name = nullptr);
    
    /**
     * @brief Destructor - disconnects from X11
     */
    ~X11Connection();
    
    // Non-copyable, non-movable
    X11Connection(const X11Connection&) = delete;
    X11Connection& operator=(const X11Connection&) = delete;
    
    /**
     * @brief Check if connection is valid
     */
    bool isValid() const noexcept { return connection_ != nullptr && !hasError(); }
    
    /**
     * @brief Check for connection error
     */
    bool hasError() const noexcept;
    
    /**
     * @brief Get the XCB connection
     */
    xcb_connection_t* get() noexcept { return connection_; }
    const xcb_connection_t* get() const noexcept { return connection_; }
    
    /**
     * @brief Get the screen
     */
    xcb_screen_t* getScreen() noexcept { return screen_; }
    const xcb_screen_t* getScreen() const noexcept { return screen_; }
    
    /**
     * @brief Get screen number
     */
    int getScreenNumber() const noexcept { return screen_number_; }
    
    /**
     * @brief Flush pending requests
     */
    void flush();
    
    /**
     * @brief Wait for next event
     * @return Event or nullptr if error
     */
    xcb_generic_event_t* waitForEvent();
    
    /**
     * @brief Poll for event (non-blocking)
     * @return Event or nullptr if none available
     */
    xcb_generic_event_t* pollForEvent();
    
    /**
     * @brief Generate a new XID
     */
    uint32_t generateId();
    
private:
    xcb_connection_t* connection_;
    xcb_screen_t* screen_;
    int screen_number_;
};

} // namespace fensterchef

#endif // FENSTERCHEF_X11_X11CONNECTION_HPP
