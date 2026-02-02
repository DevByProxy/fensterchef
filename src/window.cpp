#include <cinttypes>
#include <cstring>

#include "configuration.h"
#include "event.h"
#include "frame.h"
#include "log.h"
#include "monitor.h"
#include "window.h"
#include "xalloc.h"

/* the window that was created before any other */
Window *oldest_window = nullptr;

/* the window at the bottom of the Z stack */
Window *bottom_window = nullptr;

/* the window at the top of the Z stack */
Window *top_window = nullptr;

/* the first window in the number linked list */
Window *first_window = nullptr;

/* the currently focused window */
Window *focus_window = nullptr;

/* Window constructor - initializes window from X window */
Window::Window(xcb_window_t xcb_window)
    : name(nullptr)
    , protocols(nullptr)
    , states(nullptr)
    , x(0)
    , y(0)
    , width(0)
    , height(0)
    , border_size(0)
    , border_color(0)
    , number(0)
    , below(nullptr)
    , above(nullptr)
    , newer(nullptr)
    , next(nullptr)
{
    xcb_get_window_attributes_cookie_t attributes_cookie;
    xcb_get_window_attributes_reply_t *attributes = nullptr;
    xcb_get_geometry_cookie_t geometry_cookie;
    xcb_get_geometry_reply_t *geometry = nullptr;
    xcb_generic_error_t *error = nullptr;

    attributes_cookie = xcb_get_window_attributes(connection, xcb_window);
    geometry_cookie = xcb_get_geometry(connection, xcb_window);

    attributes = xcb_get_window_attributes_reply(connection, attributes_cookie, &error);
    if (attributes == nullptr) {
        LOG_ERROR("could not get window attributes of %w: %E\n", xcb_window, error);
        free(error);
        xcb_discard_reply(connection, geometry_cookie.sequence);
        throw std::runtime_error("Failed to get window attributes");
    }

    geometry = xcb_get_geometry_reply(connection, geometry_cookie, &error);
    if (geometry == nullptr) {
        LOG_ERROR("could not get window geometry of %w: %E\n", xcb_window, error);
        free(attributes);
        free(error);
        throw std::runtime_error("Failed to get window geometry");
    }

    /* set the border color */
    general_values[0] = configuration.border.color;
    /* we want to know if if any properties change */
    general_values[1] = XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_change_window_attributes(connection, xcb_window,
            XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK, general_values);

    // Initialize client data
    std::memset(&client, 0, sizeof(client));
    client.id = xcb_window;
    client.x = geometry->x;
    client.y = geometry->y;
    client.width = geometry->width;
    client.height = geometry->height;
    client.border_color = configuration.border.color;
    
    /* check if the window is already mapped on the X server */
    if (attributes->map_state != XCB_MAP_STATE_UNMAPPED) {
        client.is_mapped = true;
    }

    free(geometry);
    free(attributes);

    // Initialize other members
    std::memset(&size_hints, 0, sizeof(size_hints));
    std::memset(&hints, 0, sizeof(hints));
    std::memset(&strut, 0, sizeof(strut));
    std::memset(&fullscreen_monitors, 0, sizeof(fullscreen_monitors));
    std::memset(&motif_wm_hints, 0, sizeof(motif_wm_hints));
    std::memset(&state, 0, sizeof(state));
    std::memset(&floating, 0, sizeof(floating));
    
    transient_for = XCB_NONE;

    /* start off with an invalid mode, this gets set below */
    state.mode = WINDOW_MODE_MAX;
    x = client.x;
    y = client.y;
    width = client.width;
    height = client.height;
    border_color = client.border_color;
}

/* Window destructor */
Window::~Window()
{
    // Free allocated resources
    if (name != nullptr) {
        free(name);
    }
    if (protocols != nullptr) {
        free(protocols);
    }
    if (states != nullptr) {
        free(states);
    }
}

/* Attempt to close the window */
void Window::close()
{
    time_t current_time;
    char event_data[32];
    xcb_client_message_event_t *event;

    current_time = time(nullptr);
    /* if either `WM_DELETE_WINDOW` is not supported or a close was requested
     * twice in a row
     */
    if (!supports_protocol(this, ATOM(WM_DELETE_WINDOW)) ||
            (state.was_close_requested && current_time <=
                state.user_request_close_time + REQUEST_CLOSE_MAX_DURATION)) {
        xcb_kill_client(connection, client.id);
        return;
    }

    /* bake an event for running a protocol on the window */
    event = reinterpret_cast<xcb_client_message_event_t*>(event_data);
    event->response_type = XCB_CLIENT_MESSAGE;
    event->window = client.id;
    event->type = ATOM(WM_PROTOCOLS);
    event->format = 32;
    std::memset(&event->data, 0, sizeof(event->data));
    event->data.data32[0] = ATOM(WM_DELETE_WINDOW);
    xcb_send_event(connection, false, client.id,
            XCB_EVENT_MASK_NO_EVENT, event_data);

    state.was_close_requested = true;
    state.user_request_close_time = current_time;
}

/* Get the frame this window is contained in */
Frame* Window::getFrame() const
{
    return get_frame_of_window(this);
}

/* Check if the window accepts input focus */
bool Window::acceptsFocus()
{
    if (state.mode == WINDOW_MODE_DOCK) {
        return false;
    }

    if (supports_protocol(this, ATOM(WM_TAKE_FOCUS))) {
        return true;
    }

    return !(hints.flags & XCB_ICCCM_WM_HINT_INPUT) || hints.input != 0;
}

/* Get the minimum size the window should have */
void Window::getMinimumSize(Size *size) const
{
    get_minimum_window_size(this, size);
}

/* Get the maximum size the window should have */
void Window::getMaximumSize(Size *size) const
{
    get_maximum_window_size(this, size);
}

/* Set the position and size of the window */
void Window::setSize(int32_t new_x, int32_t new_y, uint32_t new_width, uint32_t new_height)
{
    set_window_size(this, new_x, new_y, new_width, new_height);
}

/* Move the window such that it is in bounds of the screen */
void Window::placeInBounds()
{
    place_window_in_bounds(this);
}

/* Put the window on the best suited Z stack position */
void Window::updateLayer()
{
    update_window_layer(this);
}

/* Create a window and add it to the window list. */
Window *create_window(xcb_window_t xcb_window)
{
    xcb_get_window_attributes_cookie_t attributes_cookie;
    xcb_get_window_attributes_reply_t *attributes;
    xcb_generic_error_t *error;
    Window *window;
    Window *previous;
    window_mode_t mode;

    // Check if we should manage this window
    attributes_cookie = xcb_get_window_attributes(connection, xcb_window);
    attributes = xcb_get_window_attributes_reply(connection, attributes_cookie, &error);
    
    if (attributes == nullptr) {
        LOG_ERROR("could not get window attributes of %w: %E\n", xcb_window, error);
        free(error);
        return nullptr;
    }
    
    /* override redirect is used by windows to indicate that our window manager
     * should not tamper with them, we also check if the class is InputOnly
     * which is not a case we want to handle
     */
    if (attributes->override_redirect ||
            attributes->_class == XCB_WINDOW_CLASS_INPUT_ONLY) {
        free(attributes);
        return nullptr;
    }
    free(attributes);

    // Use constructor to create the window
    try {
        window = new Window(xcb_window);
    } catch (const std::exception& e) {
        LOG_ERROR("failed to create window: %s\n", e.what());
        return nullptr;
    }

    /* link into the Z, age and number linked lists */
    if (first_window == nullptr) {
        oldest_window = window;
        bottom_window = window;
        top_window = window;
        first_window = window;
        window->number = FIRST_WINDOW_NUMBER;
    } else {
        previous = first_window;
        if (first_window->number == FIRST_WINDOW_NUMBER) {
            /* find a gap in the window numbers */
            for (; previous->next != nullptr; previous = previous->next) {
                if (previous->number + 1 < previous->next->number) {
                    break;
                }
            }
            window->number = previous->number + 1;
            window->next = previous->next;
            previous->next = window;
        } else {
            window->number = FIRST_WINDOW_NUMBER;
            window->next = first_window;
            first_window = window;
        }

        /* put the window at the top of the Z linked list */
        while (previous->above != nullptr) {
            previous = previous->above;
        }
        previous->above = window;
        window->below = previous;

        /* put the window into the age linked list */
        previous = oldest_window;
        while (previous->newer != nullptr) {
            previous = previous->newer;
        }
        previous->newer = window;
    }

    /* initialize the window mode and Z position */
    mode = initialize_window_properties(window);
    set_window_mode(window, mode);
    window->updateLayer();

    has_client_list_changed = true;

    LOG("created new window %W\n", window);
    return window;
}

/* Attempt to close a window. If it is the first time, use a friendly method by
 * sending a close request to the window. Call this function again within
 * `REQUEST_CLOSE_MAX_DURATION` to forcefully kill it.
 */
void close_window(Window *window)
{
    time_t current_time;
    char event_data[32];
    xcb_client_message_event_t *event;

    current_time = time(nullptr);
    /* if either `WM_DELETE_WINDOW` is not supported or a close was requested
     * twice in a row
     */
    if (!supports_protocol(window, ATOM(WM_DELETE_WINDOW)) ||
            (window->state.was_close_requested && current_time <=
                window->state.user_request_close_time +
                    REQUEST_CLOSE_MAX_DURATION)) {
        xcb_kill_client(connection, window->client.id);
        return;
    }

    /* bake an event for running a protocol on the window */
    event = (xcb_client_message_event_t*) event_data;
    event->response_type = XCB_CLIENT_MESSAGE;
    event->window = window->client.id;
    event->type = ATOM(WM_PROTOCOLS);
    event->format = 32;
    memset(&event->data, 0, sizeof(event->data));
    event->data.data32[0] = ATOM(WM_DELETE_WINDOW);
    xcb_send_event(connection, false, window->client.id,
            XCB_EVENT_MASK_NO_EVENT, event_data);

    window->state.was_close_requested = true;
    window->state.user_request_close_time = current_time;
}

/* Remove @window from the Z linked list. */
static void unlink_window_from_z_list(Window *window)
{
    if (window->below != nullptr) {
        window->below->above = window->above;
    }
    if (window->above != nullptr) {
        window->above->below = window->below;
    }

    if (window == bottom_window) {
        bottom_window = window->above;
    }
    if (window == top_window) {
        top_window = window->below;
    }

    window->above = nullptr;
    window->below = nullptr;
}

/* Destroys given window and removes it from the window linked list. */
void destroy_window(Window *window)
{
    Frame *frame;
    Window *previous;

    /* really make sure the window is hidden, not sure if this case can ever
     * happen because usually a MapUnnotify event hides the window beforehand
     */
    hide_window_abruptly(window);

    /* exceptional state, this should never happen */
    if (window == focus_window) {
        focus_window = nullptr;
        LOG_ERROR("destroying window with focus\n");
    }

    /* this should also never happen but we check just in case */
    frame = get_frame_of_window(window);
    if (frame != nullptr) {
        frame->window = nullptr;
        LOG_ERROR("window being destroyed is still within a frame\n");
    }

    LOG("destroying window %W\n", window);

    /* remove from the z linked list */
    unlink_window_from_z_list(window);

    /* remove from the age linked list */
    if (oldest_window == window) {
        oldest_window = oldest_window->newer;
    } else {
        previous = oldest_window;
        while (previous->newer != window) {
            previous = previous->newer;
        }
        previous->newer = window->newer;
    }

    /* remove from the number linked list */
    if (first_window == window) {
        first_window = first_window->next;
    } else {
        previous = first_window;
        while (previous->next != window) {
            previous = previous->next;
        }
        previous->next = window->next;
    }

    has_client_list_changed = true;

    // Use delete to call the destructor
    delete window;
}

/* Adjust given @x and @y such that it follows the @window_gravity. */
void adjust_for_window_gravity(Monitor *monitor, int32_t *x, int32_t *y,
        uint32_t width, uint32_t height, uint32_t window_gravity)
{
    switch (window_gravity) {
    /* attach to the top left */
    case XCB_GRAVITY_NORTH_WEST:
        *x = monitor->x;
        *y = monitor->y;
        break;

    /* attach to the top */
    case XCB_GRAVITY_NORTH:
        *y = monitor->y;
        break;

    /* attach to the top right */
    case XCB_GRAVITY_NORTH_EAST:
        *x = monitor->x + monitor->width - width;
        *y = monitor->y;
        break;

    /* attach to the left */
    case XCB_GRAVITY_WEST:
        *x = monitor->x;
        break;

    /* put it into the center */
    case XCB_GRAVITY_CENTER:
        *x = monitor->x + (monitor->width - width) / 2;
        *y = monitor->y + (monitor->height - height) / 2;
        break;

    /* attach to the right */
    case XCB_GRAVITY_EAST:
        *x = monitor->x + monitor->width - width;
        break;

    /* attach to the bottom left */
    case XCB_GRAVITY_SOUTH_WEST:
        *x = monitor->x;
        *y = monitor->y + monitor->height - height;
        break;

    /* attach to the bottom */
    case XCB_GRAVITY_SOUTH:
        *y = monitor->y + monitor->height - height;
        break;

    /* attach to the bottom right */
    case XCB_GRAVITY_SOUTH_EAST:
        *x = monitor->x + monitor->width - width;
        *y = monitor->y + monitor->width - height;
        break;

    /* nothing to do */
    case XCB_GRAVITY_STATIC:
        break;
    }
}

/* Get the minimum size the window can have. */
void get_minimum_window_size(const Window *window, Size *size)
{
    uint32_t width = 0, height = 0;

    if (window->state.mode != WINDOW_MODE_TILING) {
        if ((window->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)) {
            width = window->size_hints.min_width;
            height = window->size_hints.min_height;
        }
    }
    size->width = MAX(width, WINDOW_MINIMUM_SIZE);
    size->height = MAX(height, WINDOW_MINIMUM_SIZE);
}

/* Get the maximum size the window can have. */
void get_maximum_window_size(const Window *window, Size *size)
{
    uint32_t width = UINT32_MAX, height = UINT32_MAX;

    if ((window->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)) {
        width = window->size_hints.max_width;
        height = window->size_hints.max_height;
    }
    size->width = MIN(width, WINDOW_MAXIMUM_SIZE);
    size->height = MIN(height, WINDOW_MAXIMUM_SIZE);
}

/* Move the window such that it is in bounds of the screen. */
void place_window_in_bounds(Window *window)
{
    /* do not move dock windows */
    if (window->state.mode == WINDOW_MODE_DOCK) {
        return;
    }

    /* make the window horizontally visible */
    if (window->x + (int32_t) window->width < WINDOW_MINIMUM_VISIBLE_SIZE) {
        window->x = WINDOW_MINIMUM_VISIBLE_SIZE - window->width;
    } else if (window->x + WINDOW_MINIMUM_VISIBLE_SIZE >=
            (int32_t) screen->width_in_pixels) {
        window->x = screen->width_in_pixels - WINDOW_MINIMUM_VISIBLE_SIZE;
    }
    /* make the window vertically visible */
    if (window->y + (int32_t) window->height < WINDOW_MINIMUM_VISIBLE_SIZE) {
        window->y = WINDOW_MINIMUM_VISIBLE_SIZE - window->height;
    } else if (WINDOW_MINIMUM_VISIBLE_SIZE >=
            (int32_t) screen->height_in_pixels) {
        window->y = screen->height_in_pixels - WINDOW_MINIMUM_VISIBLE_SIZE;
    }
}

/* Set the position and size of a window. */
void set_window_size(Window *window, int32_t x, int32_t y, uint32_t width,
        uint32_t height)
{
    Size minimum, maximum;

    get_minimum_window_size(window, &minimum);
    get_maximum_window_size(window, &maximum);

    /* make sure the window does not become too large or too small */
    width = MIN(width, maximum.width);
    height = MIN(height, maximum.height);
    width = MAX(width, minimum.width);
    height = MAX(height, minimum.height);

    if (window->state.mode == WINDOW_MODE_FLOATING) {
        window->floating.x = x;
        window->floating.y = y;
        window->floating.width = width;
        window->floating.height = height;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
}

/* Put the window on the best suited Z stack position. */
void update_window_layer(Window *window)
{
    if (window->state.mode == WINDOW_MODE_TILING) {
        if (window == bottom_window) {
            return;
        }

        LOG("setting window %W below all other windows\n", window);

        general_values[0] = XCB_STACK_MODE_BELOW;
        xcb_configure_window(connection, window->client.id,
                XCB_CONFIG_WINDOW_STACK_MODE, general_values);

        /* link onto the bottom of the Z linked list */
        unlink_window_from_z_list(window);
        bottom_window->below = window;
        window->above = bottom_window;
        bottom_window = window;
    } else {
        if (window == top_window) {
            return;
        }

        LOG("setting window %W above all other windows\n", window);

        general_values[0] = XCB_STACK_MODE_ABOVE;
        xcb_configure_window(connection, window->client.id,
                XCB_CONFIG_WINDOW_STACK_MODE, general_values);

        /* link onto the top of the Z linked list */
        unlink_window_from_z_list(window);
        top_window->above = window;
        window->below = top_window;
        top_window = window;
    }

    /* put windows that are transient for this window above it */
    for (Window *below = window->below; below != nullptr; ) {
        if (below->transient_for == window->client.id) {
            general_values[0] = window->client.id;
            general_values[1] = XCB_STACK_MODE_ABOVE;
            xcb_configure_window(connection, window->client.id,
                    XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                    general_values);

            unlink_window_from_z_list(below);
            if (window->above == nullptr) {
                top_window = below;
            } else {
                below->above = window->above;
            }
            below->below = window;
            window->above = below;
            below = window->below;
        } else {
            below = below->below;
        }
    }

    has_client_list_changed = true;
}

/* Get the internal window that has the associated xcb window. */
Window *get_window_of_xcb_window(xcb_window_t xcb_window)
{
    for (Window *window = first_window; window != nullptr;
            window = window->next) {
        if (window->client.id == xcb_window) {
            return window;
        }
    }
    return nullptr;
}

/* Checks if @frame contains @window and checks this for all its children. */
static Frame *find_frame_recursively(Frame *frame, const Window *window)
{
    if (frame->window == window) {
        return frame;
    }

    if (frame->left == nullptr) {
        return nullptr;
    }

    Frame *const find = find_frame_recursively(frame->left, window);
    if (find != nullptr) {
        return find;
    }
    
    return find_frame_recursively(frame->right, window);
}

/* Get the frame this window is contained in. */
Frame *get_frame_of_window(const Window *window)
{
    /* shortcut: only tiling windows are within a frame */
    if (window->state.mode != WINDOW_MODE_TILING) {
        return nullptr;
    }

    for (Monitor *monitor = first_monitor; monitor != nullptr;
            monitor = monitor->next) {
        Frame *const find = find_frame_recursively(monitor->frame, window);
        if (find != nullptr) {
            return find;
        }
    }
    return nullptr;
}

/* Check if @window accepts input focus. */
bool does_window_accept_focus(Window *window)
{
    if (window->state.mode == WINDOW_MODE_DOCK) {
        return false;
    }

    if (supports_protocol(window, ATOM(WM_TAKE_FOCUS))) {
        return true;
    }

    return !(window->hints.flags & XCB_ICCCM_WM_HINT_INPUT) ||
            window->hints.input != 0;
}

/* Remove any focus indication from @window. */
static inline void lose_focus(Window *window)
{
    xcb_atom_t state_atom;

    focus_window->border_color = configuration.border.color;

    state_atom = ATOM(_NET_WM_STATE_FOCUSED);
    remove_window_states(window, &state_atom, 1);
}

/* Set the window that is in focus to @window. */
void set_focus_window(Window *window)
{
    if (window == nullptr) {
        if (focus_window != nullptr) {
            lose_focus(focus_window);
            focus_window = nullptr;
        }
        return;
    }

    LOG("focusing window %W\n", window);

    if (!does_window_accept_focus(window)) {
        LOG("the window can not be focused\n");
        if (focus_window != nullptr) {
            lose_focus(focus_window);
            focus_window = nullptr;
        }
        return;
    }

    if (window == focus_window) {
        LOG("the window is already focused\n");
        return;
    }

    if (focus_window != nullptr) {
        lose_focus(focus_window);
    }

    focus_window = window;

    window->border_color = configuration.border.focus_color;
}
