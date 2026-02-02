#ifndef WINDOW_H
#define WINDOW_H

#include <cstdint>
#include <memory>

#include "bits/window_typedef.h"
#include "bits/frame_typedef.h"

#include "monitor.h"
#include "utility.h"
#include "window_state.h"

#include "x11_management.h"

/* the maximum width or height of a window */
constexpr uint16_t WINDOW_MAXIMUM_SIZE = UINT16_MAX;

/* the minimum width or height a window can have */
constexpr uint32_t WINDOW_MINIMUM_SIZE = 4;

/* the minimum length of the window that needs to stay visible */
constexpr uint32_t WINDOW_MINIMUM_VISIBLE_SIZE = 8;

/* the number the first window gets assigned */
constexpr uint32_t FIRST_WINDOW_NUMBER = 1;

/* time in seconds to wait for a second close */
constexpr int REQUEST_CLOSE_MAX_DURATION = 3;

/* A window is a wrapper around an X window, it is always part of a few global
 * linked list and has a unique id (number).
 */
class Window {
public:
    /* Constructor */
    Window(xcb_window_t xcb_window);
    
    /* Destructor */
    ~Window();
    
    /* Delete copy constructor and copy assignment to prevent copying */
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    /* Attempt to close the window. If it is the first time, use a friendly method by
     * sending a close request. Call this function again within
     * REQUEST_CLOSE_MAX_DURATION to forcefully kill it.
     */
    void close();
    
    /* Move the window such that it is in bounds of the screen. */
    void placeInBounds();
    
    /* Get the minimum size the window should have. */
    void getMinimumSize(Size *size) const;
    
    /* Get the maximum size the window should have. */
    void getMaximumSize(Size *size) const;
    
    /* Set the position and size of the window.
     * Note that this function clips the parameters using getMinimumSize() and
     * getMaximumSize().
     */
    void setSize(int32_t x, int32_t y, uint32_t width, uint32_t height);
    
    /* Put the window on the best suited Z stack position. */
    void updateLayer();
    
    /* Get the frame this window is contained in.
     * @return nullptr when the window is not in any frame.
     */
    Frame* getFrame() const;
    
    /* Check if the window accepts input focus. */
    bool acceptsFocus();
    
    /* Accessors for commonly accessed members */
    xcb_window_t getXcbWindowId() const { return client.id; }
    uint32_t getNumber() const { return number; }
    int32_t getX() const { return x; }
    int32_t getY() const { return y; }
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    WindowState& getState() { return state; }
    const WindowState& getState() const { return state; }
    
    /* Public members that need to be accessed by window management code */
    /* the server's view of the window */
    XClient client;

    /* window name */
    utf8_t *name;

    /* X size hints of the window */
    xcb_size_hints_t size_hints;

    /* special window manager hints */
    xcb_icccm_wm_hints_t hints;

    /* window strut (reserved region on the screen) */
    wm_strut_partial_t strut;

    /* the window this window is transient for */
    xcb_window_t transient_for;

    /* the protocols the window supports */
    xcb_atom_t *protocols;

    /* the region the window should appear at as fullscreen window */
    Extents fullscreen_monitors;

    /* the motif window manager hints */
    motif_wm_hints_t motif_wm_hints;

    /* the window states */
    xcb_atom_t *states;

    /* the window state */
    WindowState state;

    /* current window position and size */
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;

    /* size and color of the border */
    uint32_t border_size;
    uint32_t border_color;

    /* position/size when the window was in floating mode */
    Rectangle floating;

    /* the id of this window */
    uint32_t number;

    /* All windows are part of the Z ordered linked list even when they are
     * hidden now.
     *
     * The terms Z stack, Z linked list and Z stacking are used interchangeably.
     */
    /* the window above this window */
    Window *below;
    /* the window below this window */
    Window *above;

    /* The age linked list stores the windows in creation time order. */
    /* a window newer than this one */
    Window *newer;

    /* The number linked list stores the windows sorted by their number. */
    /* the next window in the linked list */
    Window *next;
};

/* the window that was created before any other */
extern Window *oldest_window;

/* the window at the bottom of the Z stack */
extern Window *bottom_window;

/* the window at the top of the Z stack */
extern Window *top_window;

/* the first window in the number linked list */
extern Window *first_window;

/* the currently focused window */
extern Window *focus_window;

/* Create a window and add it to the window list. */
Window *create_window(xcb_window_t xcb);

/* Destroy given window and removes it from the window linked list.
 * This does NOT destroy the underlying X window.
 */
void destroy_window(Window *window);

/* Adjust given @x and @y such that it follows the @window_gravity. */
void adjust_for_window_gravity(Monitor *monitor, int32_t *x, int32_t *y,
        uint32_t width, uint32_t height, uint32_t window_gravity);

/* Get the internal window that has the associated X window.
 *
 * @return nullptr when none has this X window.
 */
Window *get_window_of_xcb_window(xcb_window_t xcb_window);

/* Set the window that is in focus. */
void set_focus_window(Window *window);

#endif
