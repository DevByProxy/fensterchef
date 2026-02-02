#include "Frame.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>

namespace fensterchef {

Frame::Frame()
    : x_(0)
    , y_(0)
    , width_(0)
    , height_(0)
    , window_(nullptr)
    , parent_(nullptr)
    , left_(nullptr)
    , right_(nullptr)
    , split_direction_(FRAME_SPLIT_HORIZONTALLY)
    , previous_stashed_(nullptr)
    , next_secondary_stashed_(nullptr)
{
}

Frame::~Frame() {
    // Children are automatically cleaned up via unique_ptr
}

void Frame::setGeometry(int32_t x, int32_t y, uint32_t width, uint32_t height) noexcept {
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
}

void Frame::resize(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    setGeometry(x, y, width, height);
    
    // If this frame has children, resize them proportionally
    if (left_ && right_) {
        if (split_direction_ == FRAME_SPLIT_HORIZONTALLY) {
            // Split horizontally (left and right)
            uint32_t left_width = width / 2;
            uint32_t right_width = width - left_width;
            
            left_->resize(x, y, left_width, height);
            right_->resize(x + left_width, y, right_width, height);
        } else {
            // Split vertically (top and bottom)
            uint32_t top_height = height / 2;
            uint32_t bottom_height = height - top_height;
            
            left_->resize(x, y, width, top_height);
            right_->resize(x, y + top_height, width, bottom_height);
        }
    }
    
    // If this frame has a window, resize it
    if (window_) {
        // TODO: Apply gaps and resize the actual window
        window_->setGeometry(x_, y_, width_, height_);
    }
}

bool Frame::containsPoint(int32_t x, int32_t y) const noexcept {
    return x >= x_ && x < x_ + static_cast<int32_t>(width_) &&
           y >= y_ && y < y_ + static_cast<int32_t>(height_);
}

Frame* Frame::split(SplitDirection direction) {
    if (!isLeaf()) {
        LOG_WARN("Cannot split non-leaf frame");
        return nullptr;
    }
    
    // Create two new frames
    left_ = std::make_unique<Frame>();
    right_ = std::make_unique<Frame>();
    
    left_->parent_ = this;
    right_->parent_ = this;
    
    split_direction_ = (direction == SplitDirection::Horizontal) 
                       ? FRAME_SPLIT_HORIZONTALLY 
                       : FRAME_SPLIT_VERTICALLY;
    
    // Move the window to the left child
    if (window_) {
        left_->window_ = window_;
        window_ = nullptr;
    }
    
    // Resize children to fit
    resize(x_, y_, width_, height_);
    
    LOG_INFO("Frame split successfully");
    return right_.get();
}

void Frame::remove() {
    if (!parent_) {
        LOG_WARN("Cannot remove root frame");
        return;
    }
    
    // Get sibling
    Frame* sibling = nullptr;
    if (parent_->left_.get() == this) {
        sibling = parent_->right_.get();
    } else {
        sibling = parent_->left_.get();
    }
    
    if (!sibling) {
        LOG_ERROR("Frame has no sibling");
        return;
    }
    
    // Move sibling's content to parent
    parent_->window_ = sibling->window_;
    parent_->left_ = std::move(sibling->left_);
    parent_->right_ = std::move(sibling->right_);
    parent_->split_direction_ = sibling->split_direction_;
    
    // Update parent pointers
    if (parent_->left_) {
        parent_->left_->parent_ = parent_;
    }
    if (parent_->right_) {
        parent_->right_->parent_ = parent_;
    }
    
    // Resize parent to update layout
    parent_->resize(parent_->x_, parent_->y_, parent_->width_, parent_->height_);
}

Frame* Frame::getRoot() {
    Frame* root = this;
    while (root->parent_) {
        root = root->parent_;
    }
    return root;
}

const Frame* Frame::getRoot() const {
    const Frame* root = this;
    while (root->parent_) {
        root = root->parent_;
    }
    return root;
}

void Frame::reload() {
    if (window_) {
        // Reload window to fit frame
        // TODO: Apply gaps properly
        window_->setGeometry(x_, y_, width_, height_);
    }
    
    // Recursively reload children
    if (left_) {
        left_->reload();
    }
    if (right_) {
        right_->reload();
    }
}

void Frame::getGaps(Extents& gaps) const {
    // TODO: Get gaps from configuration
    gaps.left = 5;
    gaps.right = 5;
    gaps.top = 5;
    gaps.bottom = 5;
}

void Frame::focus() {
    if (window_) {
        window_->focus();
    } else if (left_) {
        // Focus the first leaf frame
        Frame* current = left_.get();
        while (current && !current->isLeaf()) {
            current = current->left_.get();
        }
        if (current && current->window_) {
            current->window_->focus();
        }
    }
}

Monitor* Frame::getMonitor() const {
    // TODO: Implement monitor lookup
    return nullptr;
}

} // namespace fensterchef
