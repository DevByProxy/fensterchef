#ifndef FENSTERCHEF_WINDOW_WINDOW_MANAGER_HPP
#define FENSTERCHEF_WINDOW_WINDOW_MANAGER_HPP

#include "Window.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <xcb/xcb.h>

namespace fensterchef {

/**
 * @brief Manages the lifecycle and organization of windows
 * 
 * This class replaces the global window management functions and provides
 * a clean OOP interface for window operations. It maintains window lists,
 * handles window creation/destruction, and manages focus.
 */
class WindowManager {
public:
    /**
     * @brief Construct a WindowManager
     */
    WindowManager();
    
    /**
     * @brief Destructor - cleans up all windows
     */
    ~WindowManager();
    
    // Non-copyable, non-movable
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    WindowManager(WindowManager&&) = delete;
    WindowManager& operator=(WindowManager&&) = delete;
    
    /**
     * @brief Create and register a new window
     * @param xcb_window The X11 window ID
     * @return Pointer to the created window or nullptr on failure
     */
    Window* createWindow(xcb_window_t xcb_window);
    
    /**
     * @brief Destroy and unregister a window
     * @param window The window to destroy
     */
    void destroyWindow(Window* window);
    
    /**
     * @brief Find a window by X11 ID
     * @param xcb_window The X11 window ID
     * @return Pointer to the window or nullptr if not found
     */
    Window* findWindow(xcb_window_t xcb_window) const;
    
    /**
     * @brief Find a window by number
     * @param number The window number
     * @return Pointer to the window or nullptr if not found
     */
    Window* findWindowByNumber(uint32_t number) const;
    
    /**
     * @brief Get the currently focused window
     * @return Pointer to focused window or nullptr
     */
    Window* getFocusedWindow() const noexcept { return focused_window_; }
    
    /**
     * @brief Set the focused window
     * @param window The window to focus (can be nullptr)
     */
    void setFocusedWindow(Window* window);
    
    /**
     * @brief Get all windows
     * @return Vector of all managed windows
     */
    const std::vector<std::unique_ptr<Window>>& getAllWindows() const noexcept {
        return windows_;
    }
    
    /**
     * @brief Get windows in Z-order (bottom to top)
     * @return Vector of windows in Z-order
     */
    std::vector<Window*> getWindowsInZOrder() const;
    
    /**
     * @brief Get windows by number order
     * @return Vector of windows sorted by number
     */
    std::vector<Window*> getWindowsByNumber() const;
    
    /**
     * @brief Get windows by creation order (oldest to newest)
     * @return Vector of windows in creation order
     */
    std::vector<Window*> getWindowsByAge() const;
    
    /**
     * @brief Get the total number of windows
     * @return Number of managed windows
     */
    size_t getWindowCount() const noexcept { return windows_.size(); }
    
    /**
     * @brief Move a window to the top of the Z-order
     * @param window The window to raise
     */
    void raiseWindow(Window* window);
    
    /**
     * @brief Move a window to the bottom of the Z-order
     * @param window The window to lower
     */
    void lowerWindow(Window* window);
    
    /**
     * @brief Update the Z-order of all windows based on their mode
     */
    void updateAllStackPositions();
    
    /**
     * @brief Notify that the client list has changed
     */
    void notifyClientListChanged() { client_list_changed_ = true; }
    
    /**
     * @brief Check if client list has changed and reset flag
     * @return true if changed since last check
     */
    bool pollClientListChanged() {
        bool changed = client_list_changed_;
        client_list_changed_ = false;
        return changed;
    }
    
private:
    /**
     * @brief Allocate the next available window number
     * @return The allocated number
     */
    uint32_t allocateWindowNumber();
    
    /**
     * @brief Insert window into Z-order list
     */
    void insertIntoZOrder(Window* window);
    
    /**
     * @brief Remove window from Z-order list
     */
    void removeFromZOrder(Window* window);
    
    /**
     * @brief Insert window into number-ordered list
     */
    void insertIntoNumberOrder(Window* window);
    
    /**
     * @brief Remove window from number-ordered list
     */
    void removeFromNumberOrder(Window* window);
    
    /**
     * @brief Insert window into age-ordered list
     */
    void insertIntoAgeOrder(Window* window);
    
    /**
     * @brief Remove window from age-ordered list
     */
    void removeFromAgeOrder(Window* window);
    
    // Window storage (owns the windows)
    std::vector<std::unique_ptr<Window>> windows_;
    
    // Fast lookup by XCB ID
    std::unordered_map<xcb_window_t, Window*> window_by_xcb_id_;
    
    // Ordered list heads
    Window* bottom_window_;      ///< Bottom of Z-order
    Window* top_window_;         ///< Top of Z-order
    Window* first_window_;       ///< First in number order
    Window* oldest_window_;      ///< Oldest window
    
    // Current focus
    Window* focused_window_;
    
    // State flags
    bool client_list_changed_;
    
    // Next window number
    uint32_t next_window_number_;
};

} // namespace fensterchef

#endif // FENSTERCHEF_WINDOW_WINDOW_MANAGER_HPP
