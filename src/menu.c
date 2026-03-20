#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <wctype.h>

#include <X11/XKBlib.h>

#include "configuration.h"
#include "font.h"
#include "frame.h"
#include "log.h"
#include "menu.h"
#include "notification.h"
#include "window.h"
#include "x11/display.h"

/* the maximum priority a menu item can have */
#define MAX_ITEM_PRIORITY 10000

/* the menu to choose applications from */
Menu *application_chooser;

/* the menu to choose windows from */
Menu *window_chooser;

/* input method handling */
static struct {
    /* input method and context */
    XIM method;
    XIC context;
    /* input buffer */
    utf8_t *buffer;
    int capacity;
} input;

/* Open the input method compatible with current locale. */
static void open_input_method(void)
{
    /* destroy the old input context and method */
    if (input.context != NULL) {
        XDestroyIC(input.context);
        input.context = NULL;
    }
    if (input.method != NULL) {
        XCloseIM(input.method);
    }

    /* open a connection to the input server */
    input.method = XOpenIM(display, NULL, NULL, NULL);
    if (input.method == NULL) {
        LOG_ERROR("could not open input method\n");
    } else {
        input.context = XCreateIC(input.method,
                XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                NULL);
        if (input.context == NULL) {
            LOG_ERROR("could not create input context\n");
        }
    }

    /* create a buffer for the input method */
    input.capacity = 64;
    REALLOCATE(input.buffer, input.capacity);
}

/* Callback for when a new input method is available. */
static void callback_instantiate_input_method(Display *display,
        XPointer client, XPointer call)
{
    (void) display; (void) client; (void) call;

	LOG_DEBUG("input method instantiated\n");
    open_input_method();
}

/* Create a menu for showing items the user can select from. */
Menu *create_menu(void)
{
    Menu *menu;
    XSetWindowAttributes attributes;

    ALLOCATE_ZERO(menu, 1);

    if (menu->reference.id == None) {
        menu->reference.x = -1;
        menu->reference.y = -1;
        menu->reference.width = 1;
        menu->reference.height = 1;
        menu->reference.border_width = configuration.border_size;
        menu->reference.border = configuration.border_color;
        attributes.border_pixel = menu->reference.border;
        attributes.event_mask = KeyPressMask | FocusChangeMask | ExposureMask |
            VisibilityChangeMask;
        /* indicate to not manage the window */
        attributes.override_redirect = True;
        menu->reference.id = XCreateWindow(display,
                    DefaultRootWindow(display),
                    menu->reference.x, menu->reference.y,
                    menu->reference.width, menu->reference.height,
                    menu->reference.border_width, CopyFromParent,
                    InputOutput, (Visual*) CopyFromParent,
                    CWBorderPixel | CWOverrideRedirect | CWEventMask,
                    &attributes);

        if (menu->reference.id == None) {
            LOG_ERROR("failed creating menu\n");
            free(menu);
            return NULL;
        }

        const char *const menu_name = "[fensterchef] menu";
        XStoreName(display, menu->reference.id, menu_name);
    }

    /* create an XftDraw object for text rendering */
    menu->xft_draw = XftDrawCreate(display, menu->reference.id,
            DefaultVisual(display, DefaultScreen(display)),
            DefaultColormap(display, DefaultScreen(display)));

    if (menu->xft_draw == NULL) {
        LOG_ERROR("could not create drawing context for the menu\n");
        XDestroyWindow(display, menu->reference.id);
        free(menu);
        return NULL;
    }

    if (input.method == NULL) {
        /* get a call when the input method changes */
	    XRegisterIMInstantiateCallback(display, NULL, NULL, NULL,
	            callback_instantiate_input_method, NULL);
	}
    return menu;
}

/* Render all items of a menu.
 *
 * @return ERROR if the rendering failed, OK otherwise.
 */
static int render_menu(Menu *menu)
{
    XftColor background, foreground;
    Text *input_text;
    int top_offset;
    Monitor *monitor;
    unsigned width, height;
    int y;
    int selected_y = 0, selected_height = 0;

    if (allocate_xft_color(configuration.background, &background) == ERROR) {
        return ERROR;
    }

    if (allocate_xft_color(configuration.foreground, &foreground) == ERROR) {
        free_xft_color(&background);
        return ERROR;
    }

    LOG_DEBUG("rendering menu with %u items\n",
            menu->number_of_items);

    input_text = create_text(menu->string, menu->string_length);

    top_offset = input_text->height;

    /* get the monitor the application chooser should be on */
    monitor = get_focused_monitor();

    width = MAX(input_text->width, menu->width) + configuration.text_padding;
    height = top_offset + menu->height;

    width = MIN(width, monitor->width * 4 / 5);
    height = MIN(height, monitor->height * 4 / 5);

    /* set the list position and size so it is in the top center of the
     * focused monitor
     */
    configure_client(&menu->reference,
            monitor->x + (monitor->width - width) / 2 -
                configuration.border_size, monitor->y,
            width, height, configuration.border_size,
            configuration.border_radius, configuration.border_radius_inner);

    /* change border color of the menu window */
    change_client_attributes(&menu->reference,
            configuration.foreground);

    y = top_offset;
    for (unsigned i = 0; i < menu->number_of_items; i++) {
        if (i == menu->selected) {
            selected_y = y;
            selected_height = menu->items[i].text->height;
            break;
        }
        y += menu->items[i].text->height;
    }

    LOG_DEBUG("selected item is located at %d->%d\n",
            selected_y, selected_height);

    if (menu->selected == 0) {
        /* special case so that the padding is shown at the top again */
        menu->scrolling = 0;
    } else if (selected_y - top_offset < menu->scrolling) {
        menu->scrolling = selected_y - top_offset;
    } else if (selected_y + selected_height - (int) height > menu->scrolling) {
        menu->scrolling = selected_y + selected_height - height;
    }

    y = top_offset - menu->scrolling;

    LOG_DEBUG("showing items starting from %dpx (pixel scroll=%u)\n",
            y, menu->scrolling);

    /* render the menu items */
    for (unsigned i = 0; i < menu->number_of_items; i++) {
        XftColor *background_pointer, *foreground_pointer;

        Text *const text = menu->items[i].text;

        /* only render the item if it is visible */
        if (y + (int) text->height >= top_offset) {
            /* use normal or inverted colors */
            if (i != menu->selected) {
                foreground_pointer = &foreground;
                background_pointer = &background;
            } else {
                foreground_pointer = &background;
                background_pointer = &foreground;
            }

            LOG_DEBUG("drawing list item %d in rect %r\n",
                    i, 0, y, width, text->height);

            /* draw background and text */
            XftDrawRect(menu->xft_draw, background_pointer,
                    0, y, width, text->height);
            draw_text(menu->xft_draw, foreground_pointer,
                    configuration.text_padding / 2, y, text);
        }

        y += text->height;

        /* stop rendering if there is no more space */
        if (y >= (int) height) {
            break;
        }
    }

    /* draw the top input text */
    if (input_text->height > 0) {
        LOG_DEBUG("input text: %.*s (%ux%u)\n",
                menu->string_length, menu->string,
                input_text->width, input_text->height);
        XftDrawRect(menu->xft_draw, &background,
                0, 0, width, top_offset);
        draw_text(menu->xft_draw, &foreground,
                configuration.text_padding / 2, 0, input_text);
        XftDrawRect(menu->xft_draw, &foreground,
                configuration.text_padding / 2 + input_text->width, 0,
                2, input_text->height);
    }

    destroy_text(input_text);

    free_xft_color(&foreground);
    free_xft_color(&background);

    return OK;
}

/* Handle a key press for the given menu. */
static void handle_key_press(Menu *menu, XKeyPressedEvent *event)
{
    KeySym key_symbol;

    key_symbol = XkbKeycodeToKeysym(display, event->keycode, 0, 0);
    switch (key_symbol) {
    /* go to the first item */
    case XK_Home:
        menu->selected = 0;
        break;

    /* go to the last item */
    case XK_End:
        menu->selected = menu->number_of_items - 1;
        break;

    /* go to the previous item */
    case XK_Left:
    case XK_Up:
        if (menu->selected > 0) {
            menu->selected--;
        }
        break;

    /* go to the next item */
    case XK_Right:
    case XK_Down:
        if (menu->selected + 1 < menu->number_of_items) {
            menu->selected++;
        }
        break;
    }
}

/* Compare the score/string of two menu items. */
static int compare_menu_item_score(const void *void_a, const void *void_b)
{
    struct menu_item *a, *b;

    a = (struct menu_item*) void_a;
    b = (struct menu_item*) void_b;
    if (a->score != b->score) {
        return b->score - a->score;
    } else {
        return strcmp(a->string, b->string);
    }
}

/* Update the items within the menu according to the input.
 *
 * The items that fuzzy match the input are put at the top.
 */
static void update_item_order(Menu *menu)
{
    utf8_t *old_selected_string_pointer;

    old_selected_string_pointer = menu->items[menu->selected].string;

    /* fuzzy match the input against all menu items */
    for (unsigned i = 0; i < menu->number_of_items; i++) {
        int text_index = 0, text_length;
        unsigned score = MAX_ITEM_PRIORITY;
        unsigned consecutive_score = 1;
        FcChar32 text_glyph;
        int input_index = 0;

        struct menu_item *const item = &menu->items[i];

        text_length = strlen(item->string);
        /* quick check if the input is way too long for this item */
        if ((int) menu->string_length > text_length) {
            continue;
        }

        /* fuzzy match the input against the text glyphs */
        while (input_index < (int) menu->string_length &&
                text_index < text_length) {
            const int n = FcUtf8ToUcs4((FcChar8*) &item->string[text_index],
                    &text_glyph, text_length - text_index);
            if (n <= 0) {
                break;
            }
            if (towlower(menu->string[input_index]) == towlower(text_glyph)) {
                /* additional point if the first glyph matches */
                if (text_index == 0) {
                    score++;
                }
                /* many consecutive matching letters give a bigger score */
                score += consecutive_score;
                /* additional point if the case matches */
                if (menu->string[input_index] == text_glyph) {
                    score++;
                }
                consecutive_score++;
                input_index++;
            } else {
                consecutive_score = 1;
            }
            text_index += n;
        }

        if (input_index == (int) menu->string_length) {
            item->score = score;
        } else {
            item->score = 0;
        }
        item->score += item->priority;
    }

    /* put the matching items at the top */
    SORT(menu->items, menu->number_of_items, compare_menu_item_score);

    /* keep the selected item */
    for (unsigned i = 0; i < menu->number_of_items; i++) {
        struct menu_item *const item = &menu->items[i];
        if (item->score == 0) {
            menu->selected = 0;
            break;
        }
        if (item->string == old_selected_string_pointer) {
            menu->selected = i;
            break;
        }
    }
}

/* Handle an event for the given menu.
 *
 * @return ERROR if an entry was chosen, OK otherwise.
 */
static int handle_event(Menu *menu, XEvent *event)
{
    if (!menu->reference.is_mapped) {
        return OK;
    }

    /* filter keyboard events for the input method */
    if (XFilterEvent(event, menu->reference.id)) {
        return OK;
    }

    switch (event->type) {
    /* a key was pressed */
    case KeyPress: {
        XKeyEvent *key;
    
        int length;
        KeySym key_symbol;

        key = &event->xkey;
        if (key->window != menu->reference.id) {
            break;
        }

        if (input.context == NULL) {
            length = XLookupString(key, input.buffer, input.capacity,
                    &key_symbol, NULL);
        } else {
            Status status;

            length = Xutf8LookupString(input.context, key,
                    input.buffer, input.capacity, &key_symbol, &status);
            if (status == XBufferOverflow) {
                input.capacity = length;
                REALLOCATE(input.buffer, input.capacity);
                (void) Xutf8LookupString(input.context,
                        key, input.buffer, input.capacity,
                        &key_symbol, &status);
            }
        }

        if (length == 1 && input.buffer[0] == '\x1b') {
            unmap_client(&menu->reference);
            break;
        }

        if (length == 1 && input.buffer[0] == '\r') {
            if (!(key->state & ControlMask)) {
                unmap_client(&menu->reference);
            }
            return ERROR;
        }

        if (length == 1 && input.buffer[0] == '\b') {
            if ((key->state & ControlMask)) {
                menu->string_length = 0;
            } else if (menu->string_length > 0) {
                menu->string_length--;
            }
            update_item_order(menu);
            menu->selected = 0;
        } else if (length == 0) {
            handle_key_press(menu, key);
        } else {
            FcChar32 glyph;
            int n;

            n = FcUtf8ToUcs4((FcChar8*) input.buffer, &glyph, length);
            if (n > 0) {
                LIST_APPEND_VALUE(menu->string, glyph);
                update_item_order(menu);
                menu->selected = 0;
            }
        }

        render_menu(menu);
        break;
    }

    /* focus the input context */
    case FocusIn:
        LOG_DEBUG("menu gained focus\n");
        if (input.context != NULL) {
            XSetICFocus(input.context);
        }
        break;

    /* unmap if the focus is lost */
    case FocusOut: {
        XFocusOutEvent *const focus = &event->xfocus;
        if (focus->window == menu->reference.id &&
                focus->detail != NotifyPointer) {
            LOG_DEBUG("menu lost focus\n");
            if (input.context != NULL) {
                XUnsetICFocus(input.context);
            }
            unmap_client(&menu->reference);
        }
        break;
    }

    /* re-render if needed */
    case Expose:
        render_menu(menu);
        break;

    /* put the window on top if it was covered */
    case VisibilityNotify: {
        XVisibilityEvent *const visibility = &event->xvisibility;
        if (visibility->state != VisibilityUnobscured) {
            XRaiseWindow(display, menu->reference.id);
        }
        break;
    }
    }

    return OK;
}

/* Handle an incoming X event for the application chooser. */
static void handle_application_chooser_event(XEvent *event)
{
    if (application_chooser == NULL) {
        return;
    }

    if (handle_event(application_chooser, event) == ERROR) {
        struct menu_item *const item =
            &application_chooser->items[application_chooser->selected];
        /* give this item higher priority */
        item->priority++;
        item->priority = MIN(item->priority, MAX_ITEM_PRIORITY - 1);

        char *const argv[2] = {
            item->string,
            NULL
        };
        run_program(argv);
    }
}

/* Check if @window should appear in the window chooser. */
static bool is_window_in_window_chooser(FcWindow *window)
{
    return is_window_focusable(window);
}

/* Handle an incoming X event for the window chooser. */
static void handle_window_chooser_event(XEvent *event)
{
    if (window_chooser == NULL) {
        return;
    }

    if (handle_event(window_chooser, event) == ERROR) {
        FcWindow *const selected =
            window_chooser->items[window_chooser->selected].data;
        XKeyEvent *const key = &event->xkey;
        /* if the window is not already focused and it was not destroyed while
         * the window chooser was open, show and focus it
         */
        if (selected != NULL && selected->reference.id != None &&
                selected != Window_focus) {
            /* if shift is down, show the window in the current frame */
            if ((key->state & ShiftMask) && !selected->state.is_visible) {
                stash_frame(Frame_focus);
                set_window_mode(selected, WINDOW_MODE_TILING);
            } else {
                /* put floating windows on the top */
                update_window_layer(selected);
            }
            show_window(selected);
            set_focus_window_with_frame(selected);
        }
    }
}

/* Handle an incoming X event for all menu windows. */
void handle_menu_event(XEvent *event)
{
    handle_application_chooser_event(event);
    handle_window_chooser_event(event);
}

/* Clear the memory of previous items.
 *
 * This does not free the `items` pointer.
 */
static void clear_old_items(Menu *menu)
{
    for (unsigned i = 0; i < menu->number_of_items; i++) {
        free(menu->items[i].string);
        if (menu->items[i].text != NULL) {
            destroy_text(menu->items[i].text);
            menu->items[i].text = NULL;
        }
    }
}

/* Show a menu on screen. */
static int show_menu(Menu *menu)
{
    /* toggle the visibility */
    if (menu->reference.is_mapped) {
        unmap_client(&menu->reference);
        return OK;
    }

    if (menu->number_of_items == 0) {
        return ERROR;
    }

    menu->width = 0;
    menu->height = 0;

    /* create the text objects and measure the maximum needed width/height */
    for (unsigned i = 0; i < menu->number_of_items; i++) {
        Text *text;

        if (menu->items[i].text != NULL) {
            text = menu->items[i].text;
        } else {
            FcChar32 *glyphs;
            int glyph_count;

            glyphs = get_glyphs(menu->items[i].string, -1, &glyph_count);
            text = create_text(glyphs, glyph_count);
            menu->items[i].text = text;
        }
        menu->width = MAX(menu->width, text->width);
        menu->height += text->height;
    }

    /* size the window */
    if (render_menu(menu) == ERROR) {
        LOG_ERROR("could not render menu\n");
        return ERROR;
    }

    /* show it */
    map_client_raised(&menu->reference);

    /* focus it */
    if (XGrabKeyboard(display, menu->reference.id,
                True, GrabModeAsync, GrabModeAsync,
                CurrentTime) == AlreadyGrabbed) {
        /* hide it again */
        unmap_client(&menu->reference);
    } else {
        if (input.context != NULL) {
            XSetICValues(input.context,
                XNClientWindow, menu->reference.id,
                XNFocusWindow, menu->reference.id,
                NULL);
        }
        LIST_CLEAR(menu->string);
        update_item_order(menu);
    }

    return OK;
}

/* Choose an application to run. */
int show_application_chooser(void)
{
    static long last_modified_time;

    const char *path;
    char *string, *pointer;
    bool has_changed = false;

    if (application_chooser == NULL) {
        application_chooser = create_menu();
        if (application_chooser == NULL) {
            return ERROR;
        }
    }

    /* get all paths where executables are */
    path = getenv("PATH");
    if (path == NULL) {
        path = "/usr/bin";
    }

    /* do a first pass through all colon ':' separated paths and check if any
     * directories changed
     */
    for (string = xstrdup(path), pointer = string; pointer[0] != '\0'; ) {
        char *start, *end;
        struct stat stat_buffer;

        /* get the ending ':' and replace it with a nul-terminator */
        end = strchr(pointer, ':');
        if (end == NULL) {
            end = pointer + strlen(pointer);
        }
        start = pointer;
        if (end[0] == '\0') {
            pointer = end;
        } else {
            pointer = end + 1;
            end[0] = '\0';
        }

        /* get file state and time */
        if (stat(start, &stat_buffer) == -1) {
            LOG_ERROR("could not stat %s: %s\n",
                    start, strerror(errno));
            continue;
        }

        /* check if the directory changed (meaning a file was added/removed) */
        if (last_modified_time < stat_buffer.st_mtime) {
            last_modified_time = stat_buffer.st_mtime;
            has_changed = true;
        }
    }
    free(string);

    /* if any directories changed */
    if (has_changed) {
        unsigned capacity;
        unsigned index = 0;

        clear_old_items(application_chooser);

        capacity = application_chooser->number_of_items;
        if (capacity == 0) {
            capacity = 8;
            ALLOCATE(application_chooser->items, capacity);
        }

        /* do a second pass and collect all names within the directories */
        for (string = xstrdup(path), pointer = string; pointer[0] != '\0'; ) {
            char *start, *end;
            DIR *directory;
            struct dirent *entry;

            /* get the ending ':' and replace it with a nul-terminator */
            end = strchr(pointer, ':');
            if (end == NULL) {
                end = pointer + strlen(pointer);
            }
            start = pointer;
            if (end[0] == '\0') {
                pointer = end;
            } else {
                pointer = end + 1;
                end[0] = '\0';
            }

            /* open the directory */
            directory = opendir(start);
            if (directory == NULL) {
                LOG_ERROR("could not open %s: %s\n",
                        start, strerror(errno));
                continue;
            }

            /* go through all entries not starting with '.' and add them to the
             * menu items
             */
            while (entry = readdir(directory), entry != NULL) {
                if (entry->d_name[0] == '.') {
                    continue;
                }

                if (index >= capacity) {
                    capacity *= 2;
                    REALLOCATE(application_chooser->items, capacity);
                }
                application_chooser->items[index].string =
                    xstrdup(entry->d_name);
                application_chooser->items[index].text = NULL;
                application_chooser->items[index].priority = 0;
                application_chooser->items[index].data = NULL;
                index++;
            }
        }
        free(string);

        application_chooser->number_of_items = index;
    }

    const int result = show_menu(application_chooser);
    application_chooser->selected = 0;
    return result;
}

/* Get a character indicating the window state. */
static inline char get_indicator_character(FcWindow *window)
{
    return window == Window_focus ? '*' :
            !window->state.is_visible ? '-' :
            window->state.mode == WINDOW_MODE_FLOATING ? '=' :
            window->state.mode == WINDOW_MODE_FULLSCREEN ? 'F' : '+';
}

/* Get a string representation of a window. */
static inline int get_window_string(FcWindow *window, utf8_t *buffer,
        size_t buffer_size)
{
    return snprintf(buffer, buffer_size, "%u%c%s",
            window->number, get_indicator_character(window),
            window->properties.name);
}

/* Show the window chooser on screen. */
int show_window_chooser(void)
{
    FcWindow *selected = NULL;
    unsigned index = 0, count = 0;
    utf8_t buffer[256];
    int length;

    if (window_chooser == NULL) {
        window_chooser = create_menu();
        if (window_chooser == NULL) {
            return ERROR;
        }
    }

    /* count the windows */
    for (FcWindow *window = Window_first;
            window != NULL;
            window = window->next) {
        if (!is_window_in_window_chooser(window)) {
            continue;
        }

        if (Window_focus != NULL && Window_focus == window) {
            index = count;
            selected = window;
        } else if (Window_focus == NULL && selected == NULL) {
            selected = window;
        }
        count++;
    }

    /* dereference all previously referenced windows */
    for (unsigned i = 0; i < window_chooser->number_of_items; i++) {
        /* this is NULL for the other window message */
        if (window_chooser->items[i].data != NULL) {
            dereference_window((FcWindow*) window_chooser->items[i].data);
        }
    }

    clear_old_items(window_chooser);

    /* add a single text item indicating that there are no focusable windows */
    if (count == 0) {
        REALLOCATE(window_chooser->items, 1);
        window_chooser->number_of_items = 1;

        length = snprintf(buffer, sizeof(buffer),
                    "There are %u other windows",
                Window_count) + 1;
        window_chooser->items[0].string = xmemdup(buffer, length);
        window_chooser->items[0].text = NULL;
        window_chooser->items[0].priority = 0;
        window_chooser->items[0].data = NULL;
    /* get a string representation for all windows */
    } else {
        unsigned i = 0;

        REALLOCATE(window_chooser->items, count);
        window_chooser->number_of_items = count;

        /* add all windows as string representation to the menu */
        for (FcWindow *window = Window_first;
                window != NULL;
                window = window->next) {
            if (!is_window_in_window_chooser(window)) {
                continue;
            }
            length = get_window_string(window, buffer, sizeof(buffer)) + 1;
            length = MIN(length, (int) sizeof(buffer));
            window_chooser->items[i].string = xmemdup(buffer, length);
            window_chooser->items[i].text = NULL;
            window_chooser->items[i].priority = 0;
            /* make sure the pointer stays valid */
            reference_window(window);
            window_chooser->items[i].data = window;
            i++;
        }
    }

    window_chooser->selected = index;
    return show_menu(window_chooser);
}
