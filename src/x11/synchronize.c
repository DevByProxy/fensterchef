#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <utility/list.h>

#include "binding.h"
#include "cursor.h"
#include "frame.h"
#include "log.h"
#include "window.h"
#include "x11/display.h"

/* Synchronize the window stacking order with the server. */
static void synchronize_window_stacking_order(void)
{
    FcWindow *window, *server_window;
    XWindowChanges changes;

    window = Window_top;
    server_window = Window_server_top;
    /* go through both lists from top to bottom at the same time */
    for (; window != NULL && server_window != NULL; window = window->below) {
        if (server_window == window) {
            server_window = server_window->server_below;
        } else {
            DOUBLY_RELINK_AFTER(Window_server_bottom, Window_server_top,
                    window, server_window, server_below, server_above);

            changes.stack_mode = Above;
            changes.sibling = server_window->reference.id;
            XConfigureWindow(display, window->reference.id,
                    CWStackMode | CWSibling, &changes);
            LOG("putting window %W above %W\n",
                    window, server_window);
        }
    }
}

/* Set the client list root property. */
static void synchronize_client_list(void)
{
    /* two list of window ids that is synchronized with the actual windows */
    static Window *stacking, *age;
    /* the number of allocated and actual ids */
    static unsigned capacity;
    /* the previous length of windows */
    static unsigned old_count;

    Window root;
    FcWindow *window;
    unsigned index;
    bool is_stacking_changed = false;
    bool is_age_changed = false;

    root = DefaultRootWindow(display);

    /* allocate more ids if needed or trim if there are way too many */
    if (Window_count > capacity || MAX(Window_count, 4) * 4 < capacity) {
        LOG_DEBUG("resizing client list to %u\n",
                Window_count);
        capacity = Window_count;
        REALLOCATE(stacking, capacity);
        REALLOCATE(age, capacity);
    }

    index = 0;
    for (window = Window_bottom; window != NULL; window = window->above) {
        if (index < old_count && stacking[index] != window->reference.id) {
            is_stacking_changed = true;
        }
        stacking[index] = window->reference.id;
        index++;
    }

    index = 0;
    for (window = Window_oldest; window != NULL; window = window->newer) {
        if (index < old_count && age[index] != window->reference.id) {
            is_age_changed = true;
        }
        age[index] = window->reference.id;
        index++;
    }

    /* set the _NET_CLIENT_LIST_STACKING property if it changed */
    if (Window_count < old_count || is_stacking_changed) {
        LOG_DEBUG("setting stacking list " COLOR(CYAN)
                "_NET_CLIENT_LIST_STACKING" CLEAR_COLOR "\n");
        XChangeProperty(display, root, ATOM(_NET_CLIENT_LIST_STACKING),
                XA_WINDOW, 32, PropModeReplace,
                (unsigned char*) stacking, Window_count);
    } else if (Window_count > old_count) {
        LOG_DEBUG("appending stacking list " COLOR(CYAN)
                "_NET_CLIENT_LIST_STACKING" CLEAR_COLOR "\n");
        XChangeProperty(display, root, ATOM(_NET_CLIENT_LIST_STACKING),
                XA_WINDOW, 32, PropModeAppend,
                (unsigned char*) &stacking[old_count],
                Window_count - old_count);
    }

    /* set the _NET_CLIENT_LIST property if it changed */
    if (Window_count < old_count || is_age_changed) {
        LOG_DEBUG("setting client list " COLOR(CYAN)
                "_NET_CLIENT_LIST" CLEAR_COLOR "\n");
        XChangeProperty(display, root, ATOM(_NET_CLIENT_LIST),
                XA_WINDOW, 32, PropModeReplace,
                (unsigned char*) age, Window_count);
    } else if (Window_count > old_count) {
        LOG_DEBUG("appending stacking list " COLOR(CYAN)
                "_NET_CLIENT_LIST" CLEAR_COLOR "\n");
        XChangeProperty(display, root, ATOM(_NET_CLIENT_LIST),
                XA_WINDOW, 32, PropModeAppend,
                (unsigned char*) &age[old_count],
                Window_count - old_count);
    }

    old_count = Window_count;
}

/* Recursively check if the window is contained within @frame. */
static bool is_window_part_of(FcWindow *window, Frame *frame)
{
    if (frame->left != NULL) {
        return is_window_part_of(window, frame->left) ||
            is_window_part_of(window, frame->right);
    }
    return frame->window == window;
}

/* Set the input focus to @window. This window may be `NULL`. */
static void set_input_focus(_Nullable FcWindow *window)
{
    Window focus_id = None;
    Window active_id;
    Atom state_atom;

    if (window == NULL) {
        LOG("removed focus from all windows\n");
        active_id = DefaultRootWindow(display);

        focus_id = ewmh_window;
    } else {
        active_id = window->reference.id;

        state_atom = ATOM(_NET_WM_STATE_FOCUSED);
        add_window_states(window, &state_atom, 1);

        /* if the window wants no focus itself */
        if ((window->properties.hints.flags & InputHint) &&
                window->properties.hints.input == 0) {
            send_take_focus_message(window->reference.id);

            LOG("focusing window %W by sending WM_TAKE_FOCUS\n",
                    window);
        } else {
            focus_id = window->reference.id;
        }
    }

    if (focus_id != None) {
        LOG("focusing client %w\n", focus_id);
        XSetInputFocus(display, focus_id, RevertToParent, CurrentTime);
    }

    /* set the active window root property */
    XChangeProperty(display, DefaultRootWindow(display),
            ATOM(_NET_ACTIVE_WINDOW), XA_WINDOW, 32, PropModeReplace,
            (unsigned char*) &active_id, 1);
}

/* Synchronize the local data with the X server. */
void synchronize_with_server(void)
{
    /* the last root cursor that was set */
    static Cursor root_cursor;

    Cursor cursor;
    Atom atoms[2];
    FcWindow *window;

    cursor = load_cursor(CURSOR_ROOT, NULL);
    if (cursor != root_cursor) {
        XDefineCursor(display, DefaultRootWindow(display), cursor);
        root_cursor = cursor;
    }

    /* since the strut of a monitor might have changed because a window with
     * strut got hidden or shown, we need to recompute those
     */
    reconfigure_monitor_frames();

    /* update the focused state of all windows */
    for (window = Window_first; window != NULL; window = window->next) {
        /* remove the focused state from windows that are not focused */
        if (window != Window_focus) {
            atoms[0] = ATOM(_NET_WM_STATE_FOCUSED);
            remove_window_states(window, atoms, 1);
        }
    }

    synchronize_window_stacking_order();

    synchronize_client_list();

    /* configure all visible windows and map them */
    for (window = Window_top; window != NULL; window = window->below) {
        uint32_t new_border_color;
        unsigned new_border_size;

        if (!window->state.is_visible) {
            continue;
        }

        /* set the color of the focused window */
        if (window == Window_focus) {
            new_border_color = window->border_color_focus;
        /* deeply set the colors of all windows within the focused frame */
        } else if ((Window_focus == NULL ||
                        Window_focus->state.mode == WINDOW_MODE_TILING) &&
                window->state.mode == WINDOW_MODE_TILING &&
                is_window_part_of(window, Frame_focus)) {
            new_border_color = window->border_color_focus;
        /* if the window is the top window or within the focused frame, give it
         * the active color
         */
        } else if (is_window_part_of(window, Frame_focus) ||
                (window->state.mode == WINDOW_MODE_FLOATING &&
                    window == Window_top)) {
            new_border_color = window->border_color_active;
        } else {
            new_border_color = window->border_color;
        }

        new_border_size = get_window_border_size(window);

        configure_client(&window->reference, window->x, window->y,
                window->width, window->height, new_border_size,
                window->border_radius, window->border_radius_inner);
        change_client_attributes(&window->reference, new_border_color);

        atoms[0] = ATOM(_NET_WM_STATE_HIDDEN);
        remove_window_states(window, atoms, 1);

        if (window->properties.wm_state != NormalState) {
            LOG_DEBUG("window %W is now normal\n",
                    window);

            window->properties.wm_state = NormalState;
            atoms[0] = NormalState;
            /* no icon */
            atoms[1] = None;
            XChangeProperty(display, window->reference.id, ATOM(WM_STATE),
                    ATOM(WM_STATE), 32, PropModeReplace,
                    (unsigned char*) atoms, 2);
        }

        map_client(&window->reference);
    }

    /* unmap all invisible windows */
    for (FcWindow *window = Window_bottom;
            window != NULL;
            window = window->above) {
        if (window->state.is_visible) {
            continue;
        }

        atoms[0] = ATOM(_NET_WM_STATE_HIDDEN);
        add_window_states(window, atoms, 1);

        if (window->properties.wm_state != WithdrawnState) {
            LOG_DEBUG("window %W is now withdrawn\n",
                    window);

            window->properties.wm_state = WithdrawnState;
            atoms[0] = WithdrawnState;
            /* no icon */
            atoms[1] = None;
            XChangeProperty(display, window->reference.id, ATOM(WM_STATE),
                    ATOM(WM_STATE), 32, PropModeReplace,
                    (unsigned char*) atoms, 2);
        }

        unmap_client(&window->reference);
    }

    /* set the input focus if it changed */
    if (Window_server_focus != Window_focus) {
        set_input_focus(Window_focus);
        Window_server_focus = Window_focus;
    }
}

/**********************
 * XReference methods *
 **********************/

/* Show the client on the X server. */
void map_client(XReference *reference)
{
    if (reference->is_mapped) {
        return;
    }

    LOG("showing client %w\n", reference->id);

    reference->is_mapped = true;

    XMapWindow(display, reference->id);
}

/* Show the client on the X server at the top. */
void map_client_raised(XReference *reference)
{
    if (reference->is_mapped) {
        return;
    }

    LOG("showing client %w raised\n", reference->id);

    reference->is_mapped = true;

    XMapRaised(display, reference->id);
}

/* Hide the client on the X server. */
void unmap_client(XReference *reference)
{
    if (!reference->is_mapped) {
        return;
    }

    LOG("hiding client %w\n", reference->id);

    reference->is_mapped = false;

    XUnmapWindow(display, reference->id);
}

/* Get a list of rectangles that make up a rectangle with rounded corners. */
static XRectangle *get_rectangles_for_radius(unsigned width, unsigned height,
        unsigned radius, unsigned *count)
{
    LIST(XRectangle, rectangles);
    XRectangle rectangle;
    int xi, yi, p;
    int right, bottom;

    radius = MIN(radius, width / 2);
    radius = MIN(radius, height / 2);

    LIST_INITIALIZE(rectangles, MAX(radius, 1));

    xi = radius;
    yi = 0;
    p = 1 - radius;

    right = width - 2 * radius;
    bottom = height - 2 * radius;

    /* make the middle rectangle */
    rectangle.x = 0;
    rectangle.y = radius;
    rectangle.width = width;
    rectangle.height = bottom;
    LIST_APPEND_VALUE(rectangles, rectangle);

    bottom += radius - 1;

    /* use the mid point circle algorithm to get a good circle approximation */
    while (xi > yi) {
        yi++;

        if (p > 0) {
            xi--;
            p -= 2 * xi;
        }

        if (xi < yi) {
            break;
        }

        p += 2 * yi + 1;

        /* the points of the circle are stretched out to the corners of the
         * rectangle
         */
        rectangle.x = radius - xi;
        rectangle.y = radius - yi;
        rectangle.width = right + 2 * xi;
        rectangle.height = 1;
        LIST_APPEND_VALUE(rectangles, rectangle);

        rectangle.x = radius - xi;
        rectangle.y = bottom + yi;
        rectangle.width = right + 2 * xi;
        rectangle.height = 1;
        LIST_APPEND_VALUE(rectangles, rectangle);

        if (xi != yi) {
            rectangle.x = radius - yi;
            rectangle.y = radius - xi;
            rectangle.width = right + 2 * yi;
            rectangle.height = 1;
            LIST_APPEND_VALUE(rectangles, rectangle);

            rectangle.x = radius - yi;
            rectangle.y = bottom + xi;
            rectangle.width = right + 2 * yi;
            rectangle.height = 1;
            LIST_APPEND_VALUE(rectangles, rectangle);
        }
    }

    *count = rectangles_length;

    return rectangles;
}

/* Set the size of a window associated to the X server. */
void configure_client(XReference *reference, int x, int y, unsigned width,
        unsigned height, unsigned border_width,
        unsigned border_radius, unsigned border_radius_inner)
{
    XWindowChanges changes;
    unsigned mask = 0;
    bool is_radius_changed = false, is_inner_radius_changed = false;

    if (reference->x != x) {
        reference->x = x;
        changes.x = x;
        mask |= CWX;
    }

    if (reference->y != y) {
        reference->y = y;
        changes.y = y;
        mask |= CWY;
    }

    if (reference->width != width) {
        reference->width = width;
        changes.width = width;
        mask |= CWWidth;
    }

    if (reference->height != height) {
        reference->height = height;
        changes.height = height;
        mask |= CWHeight;
    }

    if (reference->border_width != border_width) {
        reference->border_width = border_width;
        changes.border_width = border_width;
        mask |= CWBorderWidth;
    }

    if (reference->border_radius != border_radius) {
        reference->border_radius = border_radius;
        is_radius_changed = true;
    }

    if (reference->border_radius_inner != border_radius_inner) {
        reference->border_radius_inner = border_radius_inner;
        is_inner_radius_changed = true;
    }

    /* only allow geometry changes for InputOnly windows */
    if (reference->is_input_only) {
        mask &= (CWX | CWY | CWWidth | CWHeight);
    } else {
        if ((mask & (CWWidth | CWHeight | CWBorderWidth)) ||
                is_radius_changed) {
            XRectangle *rectangles;
            unsigned count;

            rectangles = get_rectangles_for_radius(width + 2 * border_width,
                    height + 2 * border_width, border_radius, &count);
            XShapeCombineRectangles(display, reference->id, ShapeBounding,
                    -border_width, -border_width,
                    rectangles, count, ShapeSet, 0);
            free(rectangles);
        }

        if ((mask & (CWWidth | CWHeight)) || is_inner_radius_changed) {
            XRectangle *rectangles;
            unsigned count;

            rectangles = get_rectangles_for_radius(width, height,
                    border_radius_inner, &count);
            XShapeCombineRectangles(display, reference->id, ShapeClip,
                    0, 0,
                    rectangles, count, ShapeSet, 0);
            free(rectangles);
        }
    }

    if (mask != 0) {
        LOG("configuring client %w to %r %u\n",
                reference->id, x, y, width, height, border_width);
        XConfigureWindow(display, reference->id, mask, &changes);
    }
}

/* Set the client border color. */
void change_client_attributes(XReference *reference, unsigned border_color)
{
    XSetWindowAttributes attributes;
    unsigned mask = 0;

    if (reference->is_input_only) {
        return;
    }

    if (reference->border != border_color) {
        reference->border = border_color;
        attributes.border_pixel = border_color;
        mask |= CWBorderPixel;
    }

    if (mask != 0) {
        LOG("changing attributes of client %w to " COLOR(GREEN) "#%08x\n",
                reference->id, border_color);
        XChangeWindowAttributes(display, reference->id, mask, &attributes);
    }
}

/* Forcefully update the shape of @reference according to its members. */
void update_client_shape(XReference *reference)
{
    unsigned width, height, border_width;
    XRectangle *rectangles;
    unsigned count;

    width = reference->width;
    height = reference->height;
    border_width = reference->border_width;

    rectangles = get_rectangles_for_radius(width + 2 * border_width,
            height + 2 * border_width, reference->border_radius, &count);
    XShapeCombineRectangles(display, reference->id, ShapeBounding,
            -border_width, -border_width,
            rectangles, count, ShapeSet, 0);
    free(rectangles);

    rectangles = get_rectangles_for_radius(width, height,
            reference->border_radius_inner, &count);
    XShapeCombineRectangles(display, reference->id, ShapeClip,
            0, 0,
            rectangles, count, ShapeSet, 0);
    free(rectangles);
}
