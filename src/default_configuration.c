#include <X11/keysym.h>

#include "default_configuration.h"
#include "utility.h"
#include "utf8.h"

/* the default configuration */
static const struct configuration default_configuration = {
    /* default general settings */
    .general = {
        .overlap_percentage = 80
    },

    /* default tiling settings: fill empty frames but never automatically remove
     * them
     */
    .tiling = {
        .auto_remove_void = false,
        .auto_fill_void = true
    },

    /* default font settings: Mono */
    .font = {
        .name = (utf8_t*) "Mono"
    },

    /* default border settings: no borders */
    .border = {
        .size = 0,
    },

    /* default gap settings: no gaps */
    .gaps = {
        .inner = { 0, 0, 0, 0 },
        .outer = { 0, 0, 0, 0 }
    },

    /* default notification settings */
    .notification = {
        .duration = 2,
        .padding = 6,
        .border_color = 0x000000,
        .border_size = 1,
        .foreground = 0x000000,
        .background = 0xffffff,
    },

    /* default mouse settings: Mod4 as main modifier, see
     * `merge_with_default_button_bindings()` for the default mouse bindings
     */
    .mouse = {
        .resize_tolerance = 8,
        .modifiers = XCB_MOD_MASK_4,
        .ignore_modifiers = XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2 |
            XCB_MOD_MASK_3 | XCB_MOD_MASK_5,
    },

    /* default key settings: Mod4 as main modifier, see
     * `merge_with_default_key_bindings()` for the default key bindings
     */
    .keyboard = {
        .modifiers = XCB_MOD_MASK_4,
        .ignore_modifiers = XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2 |
            XCB_MOD_MASK_3 | XCB_MOD_MASK_5,
    },
};

/* Puts the mousebindings of the default configuration into @configuration
 * without overwriting any mousebindings.
 */
void merge_with_default_button_bindings(struct configuration *configuration)
{
    /* default mousebindings, note that the modifiers are combined with
     * @configuration->mouse.modifiers
     */
    struct {
        /* the modifiers of the button */
        uint16_t modifiers;
        /* the binding flags */
        uint16_t flags;
        /* the button to press */
        xcb_button_t button_index;
        /* the singular action to execute */
        Action action;
    } default_bindings[] = {
        /* start moving or resizing a window (depends on the mouse position) */
        { 0, 0, 1, { .code = ACTION_INITIATE_RESIZE } },
        /* minimize (hide) a window */
        { 0, 0, 2, { .code = ACTION_MINIMIZE_WINDOW } },
        /* start moving a window */
        { 0, 0, 3, { .code = ACTION_INITIATE_MOVE } },
    };

    struct configuration_button *button;
    uint32_t new_count;
    struct configuration_button *next_button;

    /* get the number of buttons not defined yet */
    new_count = configuration->mouse.number_of_buttons;
    for (uint32_t i = 0; i < SIZE(default_bindings); i++) {
        const uint16_t modifiers = default_bindings[i].modifiers |
            configuration->mouse.modifiers;
        button = find_configured_button(configuration, modifiers,
                default_bindings[i].button_index, default_bindings[i].flags);
        if (button != NULL) {
            continue;
        }
        new_count++;
    }

    /* shortcut: no buttons need to be added */
    if (new_count == configuration->mouse.number_of_buttons) {
        return;
    }

    /* add the new buttons on top of the already defined buttons */
    RESIZE(configuration->mouse.buttons, new_count);
    next_button =
        &configuration->mouse.buttons[configuration->mouse.number_of_buttons];
    for (uint32_t i = 0; i < SIZE(default_bindings); i++) {
        const uint16_t modifiers = default_bindings[i].modifiers |
            configuration->mouse.modifiers;
        button = find_configured_button(configuration, modifiers,
                default_bindings[i].button_index, default_bindings[i].flags);
        if (button != NULL) {
            continue;
        }
        next_button->flags = default_bindings[i].flags;
        next_button->modifiers = modifiers;
        next_button->index = default_bindings[i].button_index;
        next_button->actions = xmemdup(&default_bindings[i].action,
                sizeof(default_bindings[i].action));
        next_button->number_of_actions = 1;
        duplicate_data_value(get_action_data_type(next_button->actions[0].code),
                    &next_button->actions[0].parameter);
        next_button++;
    }
    configuration->mouse.number_of_buttons = new_count;
}

/* Puts the keybindings of the default configuration into @configuration without
 * overwriting any keybindings.
 */
void merge_with_default_key_bindings(struct configuration *configuration)
{
    /* default keybindings, note that the modifiers are combined with
     * @configuration->keyboard.modifiers
     */
    struct {
        /* the modifiers of the key */
        uint16_t modifiers;
        /* the binding flags */
        uint16_t flags;
        /* the key symbol */
        xcb_keysym_t key_symbol;
        /* the singular action to execute */
        Action action;
    } default_bindings[] = {
        /* reload the configuration */
        { XCB_MOD_MASK_SHIFT, 0, XK_r, { .code = ACTION_RELOAD_CONFIGURATION } },

        /* move the focus to a child or parent frame */
        { 0, 0, XK_a, { .code = ACTION_PARENT_FRAME } },
        { 0, 0, XK_b, { .code = ACTION_CHILD_FRAME } },
        { XCB_MOD_MASK_SHIFT, 0, XK_a, { .code = ACTION_ROOT_FRAME } },

        /* close the active window */
        { 0, 0, XK_q, { .code = ACTION_CLOSE_WINDOW } },

        /* minimize the active window */
        { 0, 0, XK_minus, { .code = ACTION_MINIMIZE_WINDOW } },

        /* go to the next window in the tiling */
        { 0, 0, XK_n, { .code = ACTION_NEXT_WINDOW } },
        { 0, 0, XK_p, { .code = ACTION_PREVIOUS_WINDOW } },

        /* remove the current tiling frame */
        { 0, 0, XK_r, { .code = ACTION_REMOVE_FRAME } },

        /* toggle between tiling and the previous mode */
        { XCB_MOD_MASK_SHIFT, 0, XK_space, { .code = ACTION_TOGGLE_TILING } },

        /* toggle between fullscreen and the previous mode */
        { 0, 0, XK_f, { .code = ACTION_TOGGLE_FULLSCREEN } },

        /* focus from tiling to non tiling and vise versa */
        { 0, 0, XK_space, { .code = ACTION_TOGGLE_FOCUS } },

        /* split a frame */
        { 0, 0, XK_v, { .code = ACTION_SPLIT_HORIZONTALLY } },
        { 0, 0, XK_s, { .code = ACTION_SPLIT_VERTICALLY } },

        /* move between frames */
        { 0, 0, XK_k, { .code = ACTION_FOCUS_UP } },
        { 0, 0, XK_h, { .code = ACTION_FOCUS_LEFT } },
        { 0, 0, XK_l, { .code = ACTION_FOCUS_RIGHT } },
        { 0, 0, XK_j, { .code = ACTION_FOCUS_DOWN } },

        /* exchange frames */
        { XCB_MOD_MASK_SHIFT, 0, XK_k, { .code = ACTION_EXCHANGE_UP } },
        { XCB_MOD_MASK_SHIFT, 0, XK_h, { .code = ACTION_EXCHANGE_LEFT } },
        { XCB_MOD_MASK_SHIFT, 0, XK_l, { .code = ACTION_EXCHANGE_RIGHT } },
        { XCB_MOD_MASK_SHIFT, 0, XK_j, { .code = ACTION_EXCHANGE_DOWN } },

        /* resizing the top/left edges of a window */
        { XCB_MOD_MASK_CONTROL, 0, XK_Left, { ACTION_RESIZE_BY, {
                .quad = { 20, 0, 0, 0 } } } },
        { XCB_MOD_MASK_CONTROL, 0, XK_Up, { ACTION_RESIZE_BY, {
                .quad = { 0, 20, 0, 0 } } } },
        { XCB_MOD_MASK_CONTROL, 0, XK_Right, { ACTION_RESIZE_BY, {
                .quad = { -20, 0, 0, 0 } } } },
        { XCB_MOD_MASK_CONTROL, 0, XK_Down, { ACTION_RESIZE_BY, {
                .quad = { 0, -20, 0, 0 } } } },

        /* resizing the bottom/right edges of a window */
        { XCB_MOD_MASK_SHIFT, 0, XK_Left, { ACTION_RESIZE_BY, {
                .quad = { 0, 0, -20, 0 } } } },
        { XCB_MOD_MASK_SHIFT, 0, XK_Up, { ACTION_RESIZE_BY, {
                .quad = { 0, 0, 0, -20 } } } },
        { XCB_MOD_MASK_SHIFT, 0, XK_Right, { ACTION_RESIZE_BY, {
                .quad = { 0, 0, 20, 0 } } } },
        { XCB_MOD_MASK_SHIFT, 0, XK_Down, { ACTION_RESIZE_BY, {
                .quad = { 0, 0, 0, 20 } } } },

        /* move a window */
        { 0, 0, XK_Left, { ACTION_RESIZE_BY, {
                .quad = { 20, 0, -20, 0 } } } },
        { 0, 0, XK_Up, { ACTION_RESIZE_BY, {
                .quad = { 0, 20, 0, -20 } } } },
        { 0, 0, XK_Right, { ACTION_RESIZE_BY, {
                .quad = { -20, 0, 20, 0 } } } },
        { 0, 0, XK_Down, { ACTION_RESIZE_BY, {
                .quad = { 0, -20, 0, 20 } } } },

        /* inflate/deflate a window */
        { XCB_MOD_MASK_CONTROL, 0, XK_plus, { ACTION_RESIZE_BY, {
                .quad = { 10, 10, 10, 10 } } } },
        { XCB_MOD_MASK_CONTROL, 0, XK_minus, { ACTION_RESIZE_BY, {
                .quad = { -10, -10, -10, -10 } } } },
        { XCB_MOD_MASK_CONTROL, 0, XK_equal, { ACTION_RESIZE_BY, {
                .quad = { 10, 10, 10, 10 } } } },

        /* show the interactive window list */
        { 0, 0, XK_w, { .code = ACTION_SHOW_WINDOW_LIST } },

        /* run the terminal or xterm as fall back */
        { 0, 0, XK_Return, { ACTION_RUN, {
                .string = (uint8_t*)
                    "[ -n \"$TERMINAL\" ] && exec \"$TERMINAL\" || exec xterm"
            } } },

        /* quit fensterchef */
        { XCB_MOD_MASK_CONTROL | XCB_MOD_MASK_SHIFT, 0, XK_e,
            { .code = ACTION_QUIT } }
    };

    struct configuration_key *key;
    uint32_t new_count;
    struct configuration_key *next_key;

    /* get the number of keys not defined yet */
    new_count = configuration->keyboard.number_of_keys;
    for (uint32_t i = 0; i < SIZE(default_bindings); i++) {
        const uint16_t modifiers = default_bindings[i].modifiers |
            configuration->keyboard.modifiers;
        key = find_configured_key(configuration, modifiers,
                default_bindings[i].key_symbol, default_bindings[i].flags);
        if (key != NULL) {
            continue;
        }
        new_count++;
    }

    /* shortcut: no keys need to be added */
    if (new_count == configuration->keyboard.number_of_keys) {
        return;
    }

    /* add the new keys on top of the already defined keys */
    RESIZE(configuration->keyboard.keys, new_count);
    next_key =
        &configuration->keyboard.keys[configuration->keyboard.number_of_keys];
    for (uint32_t i = 0; i < SIZE(default_bindings); i++) {
        const uint16_t modifiers = default_bindings[i].modifiers |
            configuration->keyboard.modifiers;
        key = find_configured_key(configuration, modifiers,
                default_bindings[i].key_symbol, default_bindings[i].flags);
        if (key != NULL) {
            continue;
        }
        next_key->flags = default_bindings[i].flags;
        next_key->modifiers = modifiers;
        next_key->key_symbol = default_bindings[i].key_symbol;
        next_key->actions = xmemdup(&default_bindings[i].action,
                sizeof(default_bindings[i].action));
        next_key->number_of_actions = 1;
        duplicate_data_value(get_action_data_type(next_key->actions[0].code),
                    &next_key->actions[0].parameter);
        next_key++;
    }
    configuration->keyboard.number_of_keys = new_count;
}

/* Load the default values into the configuration. */
void load_default_configuration(void)
{
    struct configuration configuration;

    /* create a duplicate of the default configuration */
    configuration = default_configuration;
    duplicate_configuration(&configuration);

    /* add the default bindings */
    merge_with_default_button_bindings(&configuration);
    merge_with_default_key_bindings(&configuration);

    set_configuration(&configuration);
}
