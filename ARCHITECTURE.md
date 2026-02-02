# Fensterchef Architecture Documentation

## Overview
This document describes the modern C++ architecture of Fensterchef window manager after complete restructuring.

## Design Principles

### 1. **Modularity**
The codebase is organized into clear modules, each with a specific responsibility:
- `core/` - Main application logic and coordination
- `window/` - Window management and lifecycle
- `layout/` - Tiling layout and frame management
- `display/` - Monitor and display management
- `input/` - Input handling and key bindings
- `x11/` - X11 protocol integration layer
- `utils/` - Utility classes and types

### 2. **Encapsulation**
All classes use proper encapsulation:
- **Private data members** - No public data access
- **Public interfaces** - Clean getter/setter methods
- **Const correctness** - Const methods where appropriate
- **RAII** - Automatic resource management

### 3. **Single Responsibility**
Each class has one clear purpose:
- `Application` - Coordinates all subsystems
- `WindowManager` - Manages window lifecycle
- `TilingManager` - Handles tiling logic
- `DisplayManager` - Manages monitors
- `InputHandler` - Processes input events

### 4. **Dependency Injection**
No global variables or singletons (except Logger):
- Dependencies passed through constructors
- Application owns all subsystems
- Clear ownership and lifetime management

### 5. **Modern C++**
Uses C++17 features:
- `std::unique_ptr` for ownership
- `std::vector` and `std::unordered_map` for containers
- `std::string` for text
- Move semantics for efficiency
- `constexpr` for compile-time values
- `enum class` for type safety

## Module Structure

### Core Module (`src/core/`)
**Purpose**: Main application logic and coordination

**Classes**:
- `Application` - Main application class, owns all subsystems
  - Initializes subsystems in correct order
  - Runs the main event loop
  - Coordinates between modules

**Replaces**:
- `fensterchef.cpp`
- Parts of `main.cpp`

### Window Module (`src/window/`)
**Purpose**: Window management and lifecycle

**Classes**:
- `Window` - Represents a managed X11 window
  - Fully encapsulated (private data members)
  - Clean interface for window operations
  - Manages X11 window properties
  
- `WindowManager` - Manages all windows
  - Creates and destroys windows
  - Maintains window lists (Z-order, number order, age order)
  - Manages focus
  - Fast lookup by XCB ID

**Replaces**:
- `window.h/cpp` - Now properly encapsulated
- `window_list.h/cpp` - Integrated into WindowManager
- `window_state.h/cpp` - Window state is part of Window class
- Global window variables

**Key Improvements**:
- No global variables
- Proper ownership with `std::unique_ptr`
- Fast lookup with `std::unordered_map`
- Clean separation of concerns

### Layout Module (`src/layout/`)
**Purpose**: Tiling layout and frame management

**Classes**:
- `Frame` - Represents a rectangular frame in the layout
  - Binary tree structure for splits
  - Smart pointers for child ownership
  - Clean interface for operations
  
- `TilingManager` - Manages tiling layout
  - Frame splitting and merging
  - Layout algorithms
  - Window placement

- `FrameStack` - Frame stashing for layout persistence

**Replaces**:
- `frame.h/cpp` - Now properly encapsulated
- `tiling.h/cpp` - Logic moved to TilingManager
- `stash_frame.h/cpp` - Now FrameStack class

**Key Improvements**:
- `std::unique_ptr` for child frames (automatic cleanup)
- No manual memory management
- Clear tree structure
- Type-safe operations

### Display Module (`src/display/`)
**Purpose**: Monitor and display management

**Classes**:
- `Monitor` - Represents a physical/virtual monitor
  - Geometry information
  - Associated root frame
  
- `DisplayManager` - Manages all monitors
  - Monitor detection (RandR)
  - Monitor configuration
  - Multi-monitor support

- `Renderer` - Rendering operations
  - Font rendering
  - Visual effects

**Replaces**:
- `monitor.h/cpp` - Now properly encapsulated
- `render.h/cpp` - Refactored into Renderer class

**Key Improvements**:
- Clean monitor management
- Proper ownership
- Separated rendering concerns

### Input Module (`src/input/`)
**Purpose**: Input event handling

**Classes**:
- `InputHandler` - Processes X11 input events
  - Event dispatching
  - Action execution
  
- `KeyMap` - Key binding management
  - Key to action mapping
  - Configuration loading

- `Actions` - Action definitions and execution

**Replaces**:
- `event.h/cpp` - Refactored into InputHandler
- `keymap.h/cpp` - Now KeyMap class
- `action.h/cpp` - Now Actions class

### X11 Module (`src/x11/`)
**Purpose**: X11 protocol integration

**Classes**:
- `X11Connection` - Wraps XCB connection
  - Connection management
  - Error handling
  - Utility functions
  
- `X11Atoms` - X11 atom management
  - Atom initialization
  - Atom lookup

- `X11Utilities` - X11 helper functions

**Replaces**:
- `x11_management.h/cpp` - Refactored into multiple classes

**Key Improvements**:
- Encapsulated connection
- RAII for resources
- Type-safe interface

### Utils Module (`src/utils/`)
**Purpose**: Utility classes and types

**Classes/Types**:
- `Logger` - Modern logging facility
  - Multiple log levels
  - File and console output
  - Type-safe logging
  
- `Geometry` - Geometry types
  - `Point`, `Size`, `Rectangle`, `Extents`
  - Inline operations
  - Constexpr where possible

- `StringUtils` - String utilities
  - UTF-8 handling
  - String conversions

**Replaces**:
- `log.h/cpp` - Now Logger class
- `utility.h/cpp` - Refactored
- `utf8.h/cpp` - Now StringUtils
- `xalloc.h/cpp` - No longer needed (use new/delete)

**Key Improvements**:
- Modern logging with levels
- Type-safe geometry
- No manual memory allocation

## Class Diagram

```
Application
├── Config
├── X11Connection
│   └── X11Atoms
├── WindowManager
│   └── Window [0..*]
├── DisplayManager
│   └── Monitor [0..*]
│       └── Frame (root)
├── TilingManager
│   └── FrameStack
├── InputHandler
│   ├── KeyMap
│   └── Actions
└── Renderer
```

## Benefits of New Architecture

### 1. **Better Organization**
- Clear module boundaries
- Logical file organization
- Easy to navigate

### 2. **True OOP**
- Proper encapsulation
- No public data members
- Clean interfaces

### 3. **No Global State**
- All owned by Application
- Clear dependency flow
- Easier to test

### 4. **Memory Safety**
- Smart pointers prevent leaks
- RAII ensures cleanup
- No manual memory management

### 5. **Type Safety**
- `enum class` for strong types
- Const correctness
- Modern C++ features

### 6. **Maintainability**
- Single responsibility
- Clear ownership
- Easy to extend

### 7. **Testability**
- Dependency injection
- Mockable interfaces
- Isolated modules

## Migration Notes

### Old to New Mapping

| Old                    | New                                  |
|------------------------|--------------------------------------|
| `window.h/cpp`         | `window/Window.hpp` + `WindowManager.hpp` |
| `frame.h/cpp`          | `layout/Frame.hpp` + `TilingManager.hpp` |
| `monitor.h/cpp`        | `display/Monitor.hpp` + `DisplayManager.hpp` |
| `event.h/cpp`          | `input/InputHandler.hpp` |
| `log.h/cpp`            | `utils/Logger.hpp` |
| `fensterchef.h/cpp`    | `core/Application.hpp` |
| Global variables       | Application-owned subsystems |
| `xcalloc/free`         | `new/delete`, `std::unique_ptr` |
| `NULL`                 | `nullptr` |

### Key Changes

1. **No more global variables** - Everything owned by Application
2. **Proper classes** - Private data, public interfaces
3. **Smart pointers** - Automatic memory management
4. **Namespaces** - All in `fensterchef` namespace
5. **Modern types** - STL containers, strings
6. **Module organization** - Clear directory structure

## Future Enhancements

1. Add unit tests for each module
2. Use interfaces for better testability
3. Consider using std::optional for nullable returns
4. Add more STL usage (algorithms, etc.)
5. Consider async operations where appropriate
6. Add comprehensive documentation
7. Performance profiling and optimization

## Conclusion

The new architecture provides a solid, modern C++ foundation for Fensterchef.
It's cleaner, safer, more maintainable, and truly object-oriented.
