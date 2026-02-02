#ifndef FENSTERCHEF_LAYOUT_TILING_MANAGER_HPP
#define FENSTERCHEF_LAYOUT_TILING_MANAGER_HPP

#include "Frame.hpp"
#include <memory>

namespace fensterchef {

class Monitor;

/**
 * @brief Manages tiling layout and frame operations
 */
class TilingManager {
public:
    /**
     * @brief Constructor
     */
    TilingManager();
    
    /**
     * @brief Destructor
     */
    ~TilingManager();
    
    // Non-copyable, non-movable
    TilingManager(const TilingManager&) = delete;
    TilingManager& operator=(const TilingManager&) = delete;
    
    /**
     * @brief Get the currently focused frame
     */
    Frame* getFocusedFrame() const noexcept { return focused_frame_; }
    
    /**
     * @brief Set the focused frame
     */
    void setFocusedFrame(Frame* frame);
    
    /**
     * @brief Split the focused frame
     * @param direction Direction to split
     * @return The newly created frame or nullptr on error
     */
    Frame* splitFocusedFrame(Frame::SplitDirection direction);
    
    /**
     * @brief Remove the focused frame
     */
    void removeFocusedFrame();
    
    /**
     * @brief Navigate to parent frame
     */
    void navigateToParent();
    
    /**
     * @brief Navigate to left/top child frame
     */
    void navigateToLeftChild();
    
    /**
     * @brief Navigate to right/bottom child frame
     */
    void navigateToRightChild();
    
    /**
     * @brief Navigate to left sibling or frame on the left
     */
    void navigateLeft();
    
    /**
     * @brief Navigate to right sibling or frame on the right
     */
    void navigateRight();
    
    /**
     * @brief Navigate to top frame or frame above
     */
    void navigateUp();
    
    /**
     * @brief Navigate to bottom frame or frame below
     */
    void navigateDown();
    
    /**
     * @brief Get frame at position
     * @param x X coordinate
     * @param y Y coordinate
     * @return Frame at position or nullptr
     */
    Frame* getFrameAtPosition(int32_t x, int32_t y);
    
    /**
     * @brief Place window in focused frame
     * @param window Window to place
     */
    void placeWindowInFrame(Window* window, Frame* frame);
    
    /**
     * @brief Remove window from its frame
     * @param window Window to remove
     */
    void removeWindowFromFrame(Window* window);
    
private:
    Frame* focused_frame_;
    
    /**
     * @brief Find frame containing window recursively
     */
    Frame* findFrameWithWindow(Frame* frame, Window* window);
    
    /**
     * @brief Get sibling frame
     */
    Frame* getSibling(Frame* frame);
};

} // namespace fensterchef

#endif // FENSTERCHEF_LAYOUT_TILING_MANAGER_HPP
