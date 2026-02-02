#ifndef FENSTERCHEF_LAYOUT_FRAME_HPP
#define FENSTERCHEF_LAYOUT_FRAME_HPP

#include <cstdint>
#include <memory>

namespace fensterchef {

// Forward declarations
class Window;
class Monitor;
struct Extents;

/**
 * @brief Represents a rectangular frame in the tiling layout
 * 
 * Frames partition monitors into rectangular regions that can contain
 * windows or be further split into child frames.
 */
class Frame {
public:
    enum class SplitDirection {
        Horizontal,  ///< Split left-right
        Vertical     ///< Split top-bottom
    };
    
    enum class Edge {
        Left,
        Top,
        Right,
        Bottom
    };
    
    /**
     * @brief Construct an empty frame
     */
    Frame();
    
    /**
     * @brief Destructor
     */
    ~Frame();
    
    // Non-copyable
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    
    // Movable
    Frame(Frame&&) noexcept = default;
    Frame& operator=(Frame&&) noexcept = default;
    
    // Geometry accessors
    int32_t getX() const noexcept { return x_; }
    int32_t getY() const noexcept { return y_; }
    uint32_t getWidth() const noexcept { return width_; }
    uint32_t getHeight() const noexcept { return height_; }
    
    /**
     * @brief Set frame geometry
     */
    void setGeometry(int32_t x, int32_t y, uint32_t width, uint32_t height) noexcept;
    
    /**
     * @brief Resize frame and recursively resize children
     */
    void resize(int32_t x, int32_t y, uint32_t width, uint32_t height);
    
    /**
     * @brief Check if a point is within this frame
     */
    bool containsPoint(int32_t x, int32_t y) const noexcept;
    
    // Window management
    /**
     * @brief Get the window in this frame (if any)
     */
    Window* getWindow() const noexcept { return window_; }
    
    /**
     * @brief Set the window in this frame
     */
    void setWindow(Window* window) noexcept { window_ = window; }
    
    /**
     * @brief Check if this frame contains a window
     */
    bool hasWindow() const noexcept { return window_ != nullptr; }
    
    /**
     * @brief Reload the frame's window layout
     */
    void reload();
    
    // Tree structure
    /**
     * @brief Check if this is a leaf frame (no children)
     */
    bool isLeaf() const noexcept { return left_ == nullptr; }
    
    /**
     * @brief Get parent frame
     */
    Frame* getParent() const noexcept { return parent_; }
    
    /**
     * @brief Get left/top child
     */
    Frame* getLeft() const noexcept { return left_.get(); }
    
    /**
     * @brief Get right/bottom child
     */
    Frame* getRight() const noexcept { return right_.get(); }
    
    /**
     * @brief Get split direction
     */
    SplitDirection getSplitDirection() const noexcept { return split_direction_; }
    
    /**
     * @brief Split this frame in the given direction
     * @param direction The direction to split
     * @return Pointer to the newly created right/bottom frame
     */
    Frame* split(SplitDirection direction);
    
    /**
     * @brief Remove this frame and merge its sibling with parent
     */
    void remove();
    
    /**
     * @brief Get the root frame (frame with no parent)
     */
    Frame* getRoot();
    const Frame* getRoot() const;
    
    /**
     * @brief Get the monitor containing this frame
     */
    Monitor* getMonitor() const;
    
    /**
     * @brief Get the gaps applied to the inner window
     */
    void getGaps(Extents& gaps) const;
    
    /**
     * @brief Set input focus to this frame
     */
    void focus();
    
private:
    // Geometry
    int32_t x_;
    int32_t y_;
    uint32_t width_;
    uint32_t height_;
    
    // Content
    Window* window_;             ///< Window in this frame (if leaf)
    
    // Tree structure
    Frame* parent_;              ///< Parent frame (nullptr for root)
    std::unique_ptr<Frame> left_;   ///< Left/top child
    std::unique_ptr<Frame> right_;  ///< Right/bottom child
    SplitDirection split_direction_;
    
    // Stash list pointers (for frame stacking)
    Frame* previous_stashed_;
    Frame* next_secondary_stashed_;
    
    friend class TilingManager;
    friend class FrameStack;
};

} // namespace fensterchef

#endif // FENSTERCHEF_LAYOUT_FRAME_HPP
