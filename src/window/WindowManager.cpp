#include "WindowManager.hpp"
#include "../utils/Logger.hpp"

namespace fensterchef {

// Temporary global connection pointer
xcb_connection_t* g_connection = nullptr;

WindowManager::WindowManager()
    : bottom_window_(nullptr)
    , top_window_(nullptr)
    , first_window_(nullptr)
    , oldest_window_(nullptr)
    , focused_window_(nullptr)
    , client_list_changed_(false)
    , next_window_number_(1)
{
    LOG_INFO("WindowManager created");
}

WindowManager::~WindowManager() {
    // Clean up all windows
    windows_.clear();
    LOG_INFO("WindowManager destroyed");
}

Window* WindowManager::createWindow(xcb_window_t xcb_window) {
    // Check if window already exists
    if (window_by_xcb_id_.find(xcb_window) != window_by_xcb_id_.end()) {
        LOG_WARN("Window already managed");
        return window_by_xcb_id_[xcb_window];
    }
    
    try {
        // Create new window
        auto window = std::make_unique<Window>(xcb_window);
        Window* window_ptr = window.get();
        
        // Assign window number
        window_ptr->number_ = allocateWindowNumber();
        
        // Store window
        window_by_xcb_id_[xcb_window] = window_ptr;
        windows_.push_back(std::move(window));
        
        // Insert into ordered lists
        insertIntoZOrder(window_ptr);
        insertIntoNumberOrder(window_ptr);
        insertIntoAgeOrder(window_ptr);
        
        client_list_changed_ = true;
        
        LOG_INFO("Window created and managed");
        return window_ptr;
        
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to create window: ") + e.what());
        return nullptr;
    }
}

void WindowManager::destroyWindow(Window* window) {
    if (!window) {
        return;
    }
    
    LOG_INFO("Destroying window");
    
    // Remove from lookup
    window_by_xcb_id_.erase(window->getXcbId());
    
    // Remove from ordered lists
    removeFromZOrder(window);
    removeFromNumberOrder(window);
    removeFromAgeOrder(window);
    
    // Remove from focused if needed
    if (focused_window_ == window) {
        focused_window_ = nullptr;
    }
    
    // Remove from vector
    auto it = std::find_if(windows_.begin(), windows_.end(),
                          [window](const std::unique_ptr<Window>& w) {
                              return w.get() == window;
                          });
    
    if (it != windows_.end()) {
        windows_.erase(it);
    }
    
    client_list_changed_ = true;
}

Window* WindowManager::findWindow(xcb_window_t xcb_window) const {
    auto it = window_by_xcb_id_.find(xcb_window);
    if (it != window_by_xcb_id_.end()) {
        return it->second;
    }
    return nullptr;
}

Window* WindowManager::findWindowByNumber(uint32_t number) const {
    for (const auto& window : windows_) {
        if (window->getNumber() == number) {
            return window.get();
        }
    }
    return nullptr;
}

void WindowManager::setFocusedWindow(Window* window) {
    if (focused_window_ == window) {
        return;
    }
    
    // Unfocus old window
    if (focused_window_) {
        // TODO: Remove focus indication
    }
    
    focused_window_ = window;
    
    // Focus new window
    if (focused_window_) {
        focused_window_->focus();
    }
}

std::vector<Window*> WindowManager::getWindowsInZOrder() const {
    std::vector<Window*> result;
    Window* current = bottom_window_;
    while (current) {
        result.push_back(current);
        current = current->getAbove();
    }
    return result;
}

std::vector<Window*> WindowManager::getWindowsByNumber() const {
    std::vector<Window*> result;
    Window* current = first_window_;
    while (current) {
        result.push_back(current);
        current = current->getNext();
    }
    return result;
}

std::vector<Window*> WindowManager::getWindowsByAge() const {
    std::vector<Window*> result;
    Window* current = oldest_window_;
    while (current) {
        result.push_back(current);
        current = current->getNewer();
    }
    return result;
}

void WindowManager::raiseWindow(Window* window) {
    if (!window || window == top_window_) {
        return;
    }
    
    removeFromZOrder(window);
    
    // Put at top
    if (top_window_) {
        top_window_->setAbove(window);
        window->setBelow(top_window_);
    }
    window->setAbove(nullptr);
    top_window_ = window;
    
    if (!bottom_window_) {
        bottom_window_ = window;
    }
}

void WindowManager::lowerWindow(Window* window) {
    if (!window || window == bottom_window_) {
        return;
    }
    
    removeFromZOrder(window);
    
    // Put at bottom
    if (bottom_window_) {
        bottom_window_->setBelow(window);
        window->setAbove(bottom_window_);
    }
    window->setBelow(nullptr);
    bottom_window_ = window;
    
    if (!top_window_) {
        top_window_ = window;
    }
}

void WindowManager::updateAllStackPositions() {
    // TODO: Update actual X11 window stacking
}

uint32_t WindowManager::allocateWindowNumber() {
    return next_window_number_++;
}

void WindowManager::insertIntoZOrder(Window* window) {
    if (!bottom_window_) {
        bottom_window_ = window;
        top_window_ = window;
        window->setAbove(nullptr);
        window->setBelow(nullptr);
    } else {
        // Insert at top
        top_window_->setAbove(window);
        window->setBelow(top_window_);
        window->setAbove(nullptr);
        top_window_ = window;
    }
}

void WindowManager::removeFromZOrder(Window* window) {
    if (window->getBelow()) {
        window->getBelow()->setAbove(window->getAbove());
    }
    if (window->getAbove()) {
        window->getAbove()->setBelow(window->getBelow());
    }
    
    if (window == bottom_window_) {
        bottom_window_ = window->getAbove();
    }
    if (window == top_window_) {
        top_window_ = window->getBelow();
    }
    
    window->setAbove(nullptr);
    window->setBelow(nullptr);
}

void WindowManager::insertIntoNumberOrder(Window* window) {
    if (!first_window_) {
        first_window_ = window;
        window->setNext(nullptr);
    } else {
        // Insert in number order
        Window* prev = nullptr;
        Window* current = first_window_;
        
        while (current && current->getNumber() < window->getNumber()) {
            prev = current;
            current = current->getNext();
        }
        
        if (!prev) {
            window->setNext(first_window_);
            first_window_ = window;
        } else {
            window->setNext(prev->getNext());
            prev->setNext(window);
        }
    }
}

void WindowManager::removeFromNumberOrder(Window* window) {
    if (window == first_window_) {
        first_window_ = window->getNext();
    } else {
        Window* current = first_window_;
        while (current && current->getNext() != window) {
            current = current->getNext();
        }
        if (current) {
            current->setNext(window->getNext());
        }
    }
    window->setNext(nullptr);
}

void WindowManager::insertIntoAgeOrder(Window* window) {
    if (!oldest_window_) {
        oldest_window_ = window;
        window->setNewer(nullptr);
    } else {
        // Insert at end (newest)
        Window* current = oldest_window_;
        while (current->getNewer()) {
            current = current->getNewer();
        }
        current->setNewer(window);
        window->setNewer(nullptr);
    }
}

void WindowManager::removeFromAgeOrder(Window* window) {
    if (window == oldest_window_) {
        oldest_window_ = window->getNewer();
    } else {
        Window* current = oldest_window_;
        while (current && current->getNewer() != window) {
            current = current->getNewer();
        }
        if (current) {
            current->setNewer(window->getNewer());
        }
    }
    window->setNewer(nullptr);
}

} // namespace fensterchef
