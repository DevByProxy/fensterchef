#ifndef FENSTERCHEF_UTILS_GEOMETRY_HPP
#define FENSTERCHEF_UTILS_GEOMETRY_HPP

#include <cstdint>
#include <algorithm>

namespace fensterchef {

/**
 * @brief Represents a 2D size
 */
struct Size {
    uint32_t width{0};
    uint32_t height{0};
    
    constexpr Size() = default;
    constexpr Size(uint32_t w, uint32_t h) : width(w), height(h) {}
    
    bool operator==(const Size& other) const noexcept {
        return width == other.width && height == other.height;
    }
    
    bool operator!=(const Size& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief Represents a 2D point
 */
struct Point {
    int32_t x{0};
    int32_t y{0};
    
    constexpr Point() = default;
    constexpr Point(int32_t px, int32_t py) : x(px), y(py) {}
    
    bool operator==(const Point& other) const noexcept {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Point& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief Represents a rectangle
 */
struct Rectangle {
    int32_t x{0};
    int32_t y{0};
    uint32_t width{0};
    uint32_t height{0};
    
    constexpr Rectangle() = default;
    constexpr Rectangle(int32_t px, int32_t py, uint32_t w, uint32_t h)
        : x(px), y(py), width(w), height(h) {}
    
    /**
     * @brief Check if this rectangle contains a point
     */
    bool contains(int32_t px, int32_t py) const noexcept {
        return px >= x && px < x + static_cast<int32_t>(width) &&
               py >= y && py < y + static_cast<int32_t>(height);
    }
    
    /**
     * @brief Check if this rectangle contains a point
     */
    bool contains(const Point& point) const noexcept {
        return contains(point.x, point.y);
    }
    
    /**
     * @brief Get the area of the rectangle
     */
    uint32_t area() const noexcept {
        return width * height;
    }
    
    /**
     * @brief Check if this rectangle intersects another
     */
    bool intersects(const Rectangle& other) const noexcept {
        return !(x + static_cast<int32_t>(width) <= other.x ||
                 other.x + static_cast<int32_t>(other.width) <= x ||
                 y + static_cast<int32_t>(height) <= other.y ||
                 other.y + static_cast<int32_t>(other.height) <= y);
    }
    
    /**
     * @brief Get the intersection of two rectangles
     */
    Rectangle intersection(const Rectangle& other) const noexcept {
        int32_t x1 = std::max(x, other.x);
        int32_t y1 = std::max(y, other.y);
        int32_t x2 = std::min(x + static_cast<int32_t>(width),
                             other.x + static_cast<int32_t>(other.width));
        int32_t y2 = std::min(y + static_cast<int32_t>(height),
                             other.y + static_cast<int32_t>(other.height));
        
        if (x1 < x2 && y1 < y2) {
            return Rectangle(x1, y1, x2 - x1, y2 - y1);
        }
        return Rectangle();
    }
    
    bool operator==(const Rectangle& other) const noexcept {
        return x == other.x && y == other.y &&
               width == other.width && height == other.height;
    }
    
    bool operator!=(const Rectangle& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief Represents border/padding extents (top, right, bottom, left)
 */
struct Extents {
    uint32_t left{0};
    uint32_t right{0};
    uint32_t top{0};
    uint32_t bottom{0};
    
    constexpr Extents() = default;
    constexpr Extents(uint32_t l, uint32_t r, uint32_t t, uint32_t b)
        : left(l), right(r), top(t), bottom(b) {}
    
    /**
     * @brief Create uniform extents
     */
    static constexpr Extents uniform(uint32_t value) {
        return Extents(value, value, value, value);
    }
    
    bool operator==(const Extents& other) const noexcept {
        return left == other.left && right == other.right &&
               top == other.top && bottom == other.bottom;
    }
    
    bool operator!=(const Extents& other) const noexcept {
        return !(*this == other);
    }
};

} // namespace fensterchef

#endif // FENSTERCHEF_UTILS_GEOMETRY_HPP
