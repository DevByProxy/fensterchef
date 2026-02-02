#include "frame.h"
#include "window.h"

/* the last frame in the frame stashed linked list */
static Frame *last_stashed_frame;

/* Hide all windows in @frame and child frames. */
static void hide_inner_windows(Frame *frame)
{
    if (frame->left != nullptr) {
        hide_inner_windows(frame->left);
        hide_inner_windows(frame->right);
    } else if (frame->window != nullptr) {
        hide_window_abruptly(frame->window);
    }
}

/* Take @frame away from the screen, this leaves a singular empty frame. */
Frame *stash_frame_later(Frame *frame)
{
    /* check if it is worth saving this frame */
    if (frame->left == nullptr && frame->window == nullptr) {
        return nullptr;
    }

    /* reparent the child frames */
    Frame *const stash = xcalloc(1, sizeof(*stash));
    if (frame->left != nullptr) {
        stash->split_direction = frame->split_direction;
        stash->left = frame->left;
        stash->right = frame->right;
        stash->left->parent = stash;
        stash->right->parent = stash;

        frame->left = nullptr;
        frame->right = nullptr;
    } else {
        stash->window = frame->window;

        frame->window = nullptr;
    }
    return stash;
}

/* Links a frame into the stash linked list. */
void link_frame_into_stash(Frame *frame)
{
    if (frame == nullptr) {
        return;
    }
    frame->previous_stashed = last_stashed_frame;
    last_stashed_frame = frame;
}

/* Take @frame away from the screen, this leaves a singular empty frame. */
Frame *stash_frame(Frame *frame)
{
    hide_inner_windows(frame);

    Frame *const stash = stash_frame_later(frame);
    if (stash == nullptr) {
        return nullptr;
    }
    link_frame_into_stash(stash);
    return stash;
}

/* Check if @window still exists as hidden tiling window.
 *
 * @window may be nullptr or a completely random memory address and this function
 *         can still handle that.
 */
static bool is_window_valid(Window *window)
{
    for (Window *other = first_window; other != nullptr; other = other->next) {
        if (other == window) {
            return window->state.mode == WINDOW_MODE_TILING &&
                !window->state.is_visible;
        }
    }
    return false;
}

/* Make sure all window pointers are still valid.
 *
 * @return the number of valid windows.
 */
static uint32_t validate_inner_windows(Frame *frame)
{
    if (frame->left != nullptr) {
        return validate_inner_windows(frame->left) +
            validate_inner_windows(frame->right);
    } else if (frame->window != nullptr) {
        if (!is_window_valid(frame->window)) {
            frame->window = nullptr;
            return 0;
        }
        return 1;
    }
    return 0;
}

/* Frees @frame and all child frames. */
static void free_frame_recursively(Frame *frame)
{
    if (frame->left != nullptr) {
        free_frame_recursively(frame->left);
        free_frame_recursively(frame->right);
    }
    delete frame;
}

/* Put the child frames or window into @frame of the recently saved frame. */
Frame *pop_stashed_frame(void)
{
    Frame *pop;

    pop = last_stashed_frame;
    /* find the first valid frame in the pop list, it might be that a stashed
     * frame got invalidated because it lost all inner window and is now
     * completely empty
     */
    while (pop != nullptr) {
        if (validate_inner_windows(pop) > 0) {
            break;
        }

        Frame *const free_me = pop;
        pop = pop->previous_stashed;
        free_frame_recursively(free_me);
    }

    if (pop == nullptr) {
        last_stashed_frame = nullptr;
    } else {
        last_stashed_frame = pop->previous_stashed;
    }
    return pop;
}

/* Show all windows in @frame and child frames. */
static void show_inner_windows(Frame *frame)
{
    if (frame->left != nullptr) {
        show_inner_windows(frame->left);
        show_inner_windows(frame->right);
    } else if (frame->window != nullptr) {
        reload_frame(frame);
        frame->window->state.is_visible = true;
    }
}

/* Puts a frame from the stash into given @frame. */
void fill_void_with_stash(Frame *frame)
{
    Frame *pop;

    pop = pop_stashed_frame();
    if (pop == nullptr) {
        return;
    }
    replace_frame(frame, pop);
    show_inner_windows(frame);
    free(pop);
}
