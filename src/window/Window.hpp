#ifndef FENSTERCHEF_WINDOW_WINDOW_HPP
#define FENSTERCHEF_WINDOW_WINDOW_HPP

#include <cstdint>
#include <string>
#include <memory>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

namespace fensterchef {

// Forward declarations
class Frame;
struct Rectangle;
struct Size;
struct Extents;

/**
 * @brief Represents a managed X11 window with full encapsulation
 * 
 * This class wraps an X11 window and provides a clean OOP interface
 * for window management operations. All state is private and accessed
 * through getters/setters.
 */
class Window {
public:
    enum class Mode {
        Tiling,      ///< Window is part of the tiling layout
        Floating,    ///< Window is floating
        Fullscreen,  ///< Window is fullscreen
        Dock         ///< Window is docked (e.g., taskbar)
    };

    /**
     * @brief Construct a Window from an X11 window ID
     * @param xcb_window The X11 window identifier
     * @throws std::runtime_error if window creation fails
     */
    explicit Window(xcb_window_t xcb_window);
    
    /**
     * @brief Destructor - cleans up resources
     */
    ~Window();
    
    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    // Movable
    Window(Window&&) noexcept = default;
    Window& operator=(Window&&) noexcept = default;
    
    // Accessors
    xcb_window_t getXcbId() const noexcept { return xcb_id_; }
    uint32_t getNumber() const noexcept { return number_; }
    const std::string& getName() const noexcept { return name_; }
    
    int32_t getX() const noexcept { return x_; }
    int32_t getY() const noexcept { return y_; }
    uint32_t getWidth() const noexcept { return width_; }
    uint32_t getHeight() const noexcept { return height_; }
    
    Mode getMode() const noexcept { return mode_; }
    bool isVisible() const noexcept { return is_visible_; }
    bool isMapped() const noexcept { return is_mapped_; }
    
    uint32_t getBorderSize() const noexcept { return border_size_; }
    uint32_t getBorderColor() const noexcept { return border_color_; }
    
    // Mutators
    void setName(const std::string& name) { name_ = name; }
    void setPosition(int32_t x, int32_t y) noexcept;
    void setSize(uint32_t width, uint32_t height) noexcept;
    void setGeometry(int32_t x, int32_t y, uint32_t width, uint32_t height) noexcept;
    void setMode(Mode mode) noexcept { mode_ = mode; }
    void setVisible(bool visible) noexcept { is_visible_ = visible; }
    void setBorderColor(uint32_t color) noexcept { border_color_ = color; }
    
    // Operations
    /**
     * @brief Close the window gracefully or forcefully
     * @param force If true, forcefully kill the window
     */
    void close(bool force = false);
    
    /**
     * @brief Show the window (map it to the screen)
     */
    void show();
    
    /**
     * @brief Hide the window (unmap it from the screen)
     */
    void hide();
    
    /**
     * @brief Check if this window accepts input focus
     * @return true if the window can be focused
     */
    bool acceptsFocus() const;
    
    /**
     * @brief Set input focus to this window
     */
    void focus();
    
    /**
     * @brief Get minimum size constraints
     * @param[out] size The minimum size
     */
    void getMinimumSize(Size& size) const;
    
    /**
     * @brief Get maximum size constraints
     * @param[out] size The maximum size
     */
    void getMaximumSize(Size& size) const;
    
    /**
     * @brief Place window within screen bounds
     */
    void ensureInBounds();
    
    /**
     * @brief Update the window's Z-stack position
     */
    void updateStackPosition();
    
    // Frame association
    /**
     * @brief Get the frame containing this window
     * @return Pointer to frame or nullptr if not in a frame
     */
    Frame* getFrame() const noexcept { return frame_; }
    
    /**
     * @brief Set the frame containing this window
     * @param frame The frame to associate with
     */
    void setFrame(Frame* frame) noexcept { frame_ = frame; }
    
    // Linked list accessors (for window manager)
    Window* getAbove() const noexcept { return above_; }
    Window* getBelow() const noexcept { return below_; }
    Window* getNext() const noexcept { return next_; }
    Window* getNewer() const noexcept { return newer_; }
    
    void setAbove(Window* window) noexcept { above_ = window; }
    void setBelow(Window* window) noexcept { below_ = window; }
    void setNext(Window* window) noexcept { next_ = window; }
    void setNewer(Window* window) noexcept { newer_ = window; }
    
private:
    // X11 window ID
    xcb_window_t xcb_id_;
    
    // Window identification
    uint32_t number_;            ///< Unique window number
    std::string name_;           ///< Window title/name
    
    // Geometry
    int32_t x_;
    int32_t y_;
    uint32_t width_;
    uint32_t height_;
    
    // Visual properties
    uint32_t border_size_;
    uint32_t border_color_;
    
    // State
    Mode mode_;
    Mode previous_mode_;
    bool is_visible_;
    bool is_mapped_;
    bool close_requested_;
    time_t close_request_time_;
    
    // Frame association
    Frame* frame_;               ///< The frame containing this window (if any)
    
    // Linked list pointers (managed by WindowManager)
    Window* above_;              ///< Window above in Z-order
    Window* below_;              ///< Window below in Z-order
    Window* next_;               ///< Next in number order
    Window* newer_;              ///< Newer in creation order
    
    // X11 properties
    xcb_size_hints_t size_hints_;
    xcb_icccm_wm_hints_t wm_hints_;
    
    // Floating position (when not tiled)
    struct {
        int32_t x;
        int32_t y;
        uint32_t width;
        uint32_t height;
    } floating_geometry_;
};

} // namespace fensterchef

#endif // FENSTERCHEF_WINDOW_WINDOW_HPP
