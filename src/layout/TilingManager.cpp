#include "TilingManager.hpp"
#include "../utils/Logger.hpp"

namespace fensterchef {

TilingManager::TilingManager()
    : focused_frame_(nullptr)
{
    LOG_INFO("TilingManager created");
}

TilingManager::~TilingManager() {
    LOG_INFO("TilingManager destroyed");
}

void TilingManager::setFocusedFrame(Frame* frame) {
    if (focused_frame_ == frame) {
        return;
    }
    
    focused_frame_ = frame;
    
    if (focused_frame_) {
        focused_frame_->focus();
    }
}

Frame* TilingManager::splitFocusedFrame(Frame::SplitDirection direction) {
    if (!focused_frame_) {
        LOG_ERROR("No focused frame to split");
        return nullptr;
    }
    
    Frame* new_frame = focused_frame_->split(direction);
    
    if (new_frame) {
        // Keep focus on original frame (left child)
        if (focused_frame_->getLeft()) {
            setFocusedFrame(focused_frame_->getLeft());
        }
    }
    
    return new_frame;
}

void TilingManager::removeFocusedFrame() {
    if (!focused_frame_ || !focused_frame_->getParent()) {
        LOG_ERROR("Cannot remove focused frame");
        return;
    }
    
    Frame* parent = focused_frame_->getParent();
    focused_frame_->remove();
    
    // Focus parent or sibling
    setFocusedFrame(parent);
}

void TilingManager::navigateToParent() {
    if (focused_frame_ && focused_frame_->getParent()) {
        setFocusedFrame(focused_frame_->getParent());
    }
}

void TilingManager::navigateToLeftChild() {
    if (focused_frame_ && focused_frame_->getLeft()) {
        setFocusedFrame(focused_frame_->getLeft());
    }
}

void TilingManager::navigateToRightChild() {
    if (focused_frame_ && focused_frame_->getRight()) {
        setFocusedFrame(focused_frame_->getRight());
    }
}

void TilingManager::navigateLeft() {
    if (!focused_frame_) {
        return;
    }
    
    // Try to navigate to left sibling
    Frame* parent = focused_frame_->getParent();
    if (parent && parent->getSplitDirection() == FRAME_SPLIT_HORIZONTALLY) {
        if (parent->getRight() == focused_frame_) {
            // We're the right child, go to left child
            setFocusedFrame(parent->getLeft());
            return;
        }
    }
    
    // Otherwise try to navigate up the tree and find a frame to the left
    // This is a simplified version - full implementation would be more complex
}

void TilingManager::navigateRight() {
    if (!focused_frame_) {
        return;
    }
    
    // Try to navigate to right sibling
    Frame* parent = focused_frame_->getParent();
    if (parent && parent->getSplitDirection() == FRAME_SPLIT_HORIZONTALLY) {
        if (parent->getLeft() == focused_frame_) {
            // We're the left child, go to right child
            setFocusedFrame(parent->getRight());
            return;
        }
    }
}

void TilingManager::navigateUp() {
    if (!focused_frame_) {
        return;
    }
    
    // Try to navigate to top sibling
    Frame* parent = focused_frame_->getParent();
    if (parent && parent->getSplitDirection() == FRAME_SPLIT_VERTICALLY) {
        if (parent->getRight() == focused_frame_) {
            // We're the bottom child, go to top child
            setFocusedFrame(parent->getLeft());
            return;
        }
    }
}

void TilingManager::navigateDown() {
    if (!focused_frame_) {
        return;
    }
    
    // Try to navigate to bottom sibling
    Frame* parent = focused_frame_->getParent();
    if (parent && parent->getSplitDirection() == FRAME_SPLIT_VERTICALLY) {
        if (parent->getLeft() == focused_frame_) {
            // We're the top child, go to bottom child
            setFocusedFrame(parent->getRight());
            return;
        }
    }
}

Frame* TilingManager::getFrameAtPosition(int32_t x, int32_t y) {
    // This would need to search through all monitors' frame trees
    // Simplified for now
    if (focused_frame_) {
        Frame* root = focused_frame_->getRoot();
        
        // Recursively search for frame at position
        std::function<Frame*(Frame*)> search = [&](Frame* frame) -> Frame* {
            if (!frame || !frame->containsPoint(x, y)) {
                return nullptr;
            }
            
            if (frame->isLeaf()) {
                return frame;
            }
            
            Frame* result = search(frame->getLeft());
            if (result) return result;
            
            return search(frame->getRight());
        };
        
        return search(root);
    }
    
    return nullptr;
}

void TilingManager::placeWindowInFrame(Window* window, Frame* frame) {
    if (!window || !frame) {
        return;
    }
    
    if (!frame->isLeaf()) {
        LOG_ERROR("Cannot place window in non-leaf frame");
        return;
    }
    
    // Remove window from old frame if any
    if (window->getFrame()) {
        removeWindowFromFrame(window);
    }
    
    // Place in new frame
    frame->setWindow(window);
    window->setFrame(frame);
    
    // Update window geometry
    frame->reload();
    
    LOG_INFO("Window placed in frame");
}

void TilingManager::removeWindowFromFrame(Window* window) {
    if (!window) {
        return;
    }
    
    Frame* frame = window->getFrame();
    if (frame) {
        frame->setWindow(nullptr);
        window->setFrame(nullptr);
    }
}

Frame* TilingManager::findFrameWithWindow(Frame* frame, Window* window) {
    if (!frame) {
        return nullptr;
    }
    
    if (frame->getWindow() == window) {
        return frame;
    }
    
    if (frame->getLeft()) {
        Frame* result = findFrameWithWindow(frame->getLeft(), window);
        if (result) return result;
    }
    
    if (frame->getRight()) {
        return findFrameWithWindow(frame->getRight(), window);
    }
    
    return nullptr;
}

Frame* TilingManager::getSibling(Frame* frame) {
    if (!frame || !frame->getParent()) {
        return nullptr;
    }
    
    Frame* parent = frame->getParent();
    if (parent->getLeft() == frame) {
        return parent->getRight();
    } else {
        return parent->getLeft();
    }
}

} // namespace fensterchef
