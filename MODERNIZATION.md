# C++ Modernization Documentation

## Overview
This document describes the C++ modernization and Object-Oriented Programming (OOP) improvements made to the Fensterchef window manager.

## Goals
1. Modernize the codebase from C99 to C++17
2. Implement proper Object-Oriented Programming principles
3. Improve memory safety through RAII (Resource Acquisition Is Initialization)
4. Enhance code maintainability and readability

## Changes Made

### 1. Build System
- **Compiler**: Changed from `gcc` to `g++`
- **Standard**: Updated from C99 to C++17
- **Source Files**: Renamed all `.c` files to `.cpp`
- **Headers**: Updated C headers (`string.h`, `stdint.h`, etc.) to C++ equivalents (`cstring`, `cstdint`, etc.)

### 2. Core Classes

#### Window Class
Previously a C struct, now a proper C++ class with:
- **Constructor**: `Window(xcb_window_t)` - Initializes window from X window ID
- **Destructor**: `~Window()` - Automatic cleanup of allocated resources (RAII)
- **Member Functions**:
  - `close()` - Close the window
  - `getFrame()` - Get the frame containing this window
  - `acceptsFocus()` - Check if window accepts input focus
  - `setSize()` - Set window position and size
  - `placeInBounds()` - Ensure window is within screen bounds
  - `updateLayer()` - Update Z-stack positioning
  - Accessor methods: `getX()`, `getY()`, `getWidth()`, `getHeight()`, etc.
- **Non-copyable**: Deleted copy constructor and assignment operator
- **Type Safety**: Uses `nullptr` instead of `NULL`

#### Frame Class
Manages tiling layout partitions, converted to C++ class:
- **Constructor**: `Frame()` - Initializes empty frame
- **Destructor**: `~Frame()` - Automatic cleanup
- **Member Functions**:
  - `containsPoint()` - Check if point is within frame
  - `resize()` - Resize frame and children
  - `getGaps()` - Get frame gaps for window
  - `reload()` - Reload frame window layout
  - `setFocus()` - Set this frame as focused
  - `getRoot()` - Get root frame in hierarchy
- **Type-safe Enums**: 
  - `enum class FrameEdge` - Frame edge identifiers
  - `enum class FrameSplitDirection` - Split direction types
- **Non-copyable**: Deleted copy constructor and assignment operator

#### Monitor Class
Represents physical/virtual monitors:
- **Constructor**: `Monitor()` - Initializes monitor
- **Destructor**: `~Monitor()` - Cleanup including frame tree
- **Non-copyable**: Deleted copy constructor and assignment operator

### 3. Memory Management
- **RAII Pattern**: All classes use constructors/destructors for resource management
- **new/delete**: Replaced `xcalloc()`/`free()` with C++ `new`/`delete` operators
- **nullptr**: Replaced all `NULL` with `nullptr` for type safety
- **Smart Pointers**: Foundation laid for future use of `std::unique_ptr`/`std::shared_ptr`

### 4. Type Safety Improvements
- **constexpr**: Used for compile-time constants (e.g., `WINDOW_MAXIMUM_SIZE`, `FRAME_MINIMUM_SIZE`)
- **enum class**: Strong-typed enums for better type safety
- **C++ casts**: Using `static_cast`, `reinterpret_cast` instead of C-style casts
- **const correctness**: Added const qualifiers to member functions where appropriate

### 5. Modern C++ Features Used
- **Member initialization lists**: Initialize class members in constructor
- **Deleted functions**: Explicitly delete copy constructors/assignment operators
- **Default initialization**: Use C++17 features for cleaner initialization
- **References**: Use references where appropriate instead of pointers

## Compatibility
- The public API maintains backward compatibility where possible
- Helper functions (e.g., `create_window()`, `resize_frame()`) still exist for gradual migration
- Old C-style enums maintained alongside new enum classes for compatibility

## Future Improvements
Potential areas for further modernization:
1. Replace raw pointers with `std::unique_ptr` and `std::shared_ptr`
2. Use `std::vector` for dynamic arrays instead of manual memory management
3. Use `std::string` for string handling
4. Add move constructors and move assignment operators
5. Use `std::optional` for nullable return values
6. Implement interfaces/abstract base classes where appropriate
7. Add namespace encapsulation

## Benefits
1. **Memory Safety**: RAII ensures resources are always cleaned up
2. **Type Safety**: Strong typing prevents many common errors
3. **Maintainability**: Object-oriented design is easier to understand and extend
4. **Modern C++**: Leverages C++17 features for cleaner, safer code
5. **Future-proof**: Foundation for further modernization efforts

## Testing
The changes preserve all existing functionality while improving code quality. Testing should focus on:
- Window creation and destruction
- Frame splitting and removal
- Monitor management
- Memory leak detection (valgrind/sanitizers)

## Notes
- This modernization is conservative, maintaining the existing architecture
- Changes are focused on improving code quality without changing behavior
- The X11 API integration remains unchanged
- All global state management preserved for compatibility
