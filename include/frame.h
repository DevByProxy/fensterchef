#ifndef FRAME_H
#define FRAME_H

#include <xcb/xcb.h>
#include <cstdint>

#include "bits/frame_typedef.h"
#include "bits/window_typedef.h"

#include "utility.h"

/* the minimum width or height of a frame */
constexpr uint32_t FRAME_MINIMUM_SIZE = 12;

/* an edge of the frame */
enum class FrameEdge {
    LEFT,
    TOP,
    RIGHT,
    BOTTOM,
};

/* For backward compatibility */
typedef enum {
    FRAME_EDGE_LEFT = static_cast<int>(FrameEdge::LEFT),
    FRAME_EDGE_TOP = static_cast<int>(FrameEdge::TOP),
    FRAME_EDGE_RIGHT = static_cast<int>(FrameEdge::RIGHT),
    FRAME_EDGE_BOTTOM = static_cast<int>(FrameEdge::BOTTOM),
} frame_edge_t;

/* a direction to split a frame in */
enum class FrameSplitDirection {
    /* the frame was split horizontally (children are left and right) */
    HORIZONTALLY,
    /* the frame was split vertically (children are up and down) */
    VERTICALLY,
};

/* For backward compatibility */
typedef enum {
    FRAME_SPLIT_HORIZONTALLY = static_cast<int>(FrameSplitDirection::HORIZONTALLY),
    FRAME_SPLIT_VERTICALLY = static_cast<int>(FrameSplitDirection::VERTICALLY),
} frame_split_direction_t;

/* Frames are used to partition a monitor into multiple rectangular regions.
 *
 * When a frame has one child, it must have a second one, so either BOTH left
 * AND right are nullptr or neither are nullptr.
 * `parent` is nullptr when the frame is a root frame.
 */
class Frame {
public:
    /* Constructor */
    Frame();
    
    /* Destructor */
    ~Frame();
    
    /* Delete copy constructor and copy assignment to prevent copying */
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    
    /* Check if the given point is within this frame */
    bool containsPoint(int32_t px, int32_t py) const;
    
    /* Set the size of the frame, this also resizes the inner frames and windows */
    void resize(int32_t new_x, int32_t new_y, uint32_t new_width, uint32_t new_height);
    
    /* Get the gaps the frame applies to its inner window */
    void getGaps(Extents *gaps) const;
    
    /* Resizes the inner window to fit within the frame */
    void reload();
    
    /* Set this frame in focus, this also focuses the inner window if it exists */
    void setFocus();
    
    /* Get the frame above this one that has no parent */
    Frame* getRoot();
    
    /* Public members accessed by window management code */
    /* the window inside the frame, may be nullptr */
    Window *window;

    /* coordinates and size of the frame */
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;

    /* the direction the frame was split in */
    frame_split_direction_t split_direction;

    /* parent of the frame */
    Frame *parent;
    /* left child and right child of the frame */
    Frame *left;
    Frame *right;

    /* the previous stashed frame in the frame stashed linked list */
    Frame *previous_stashed;
    /* the next stashed frame in the secondary stashed linked list */
    Frame *next_secondary_stashed;
};

/* the currently selected/focused frame */
extern Frame *focus_frame;

/* Check if the given point is within the given frame.
 *
 * @return if the point is inside the frame.
 */
bool is_point_in_frame(const Frame *frame, int32_t x, int32_t y);

/* Get a frame at given position.
 *
 * @return a LEAF frame at given position or nullptr when there is none.
 */
Frame *get_frame_at_position(int32_t x, int32_t y);

/* Set the size of a frame, this also resize the inner frames and windows. */
void resize_frame(Frame *frame, int32_t x, int32_t y,
        uint32_t width, uint32_t height);

/* Replace @frame (windows and child frames) with @with.
 *
 * Note that this empties @with.
 */
void replace_frame(Frame *frame, Frame *with);

/* Get the gaps the frame applies to its inner window. */
void get_frame_gaps(Frame *frame, Extents *gaps);

/* Resizes the inner window to fit within the frame. */
void reload_frame(Frame *frame);

/* Set the frame in focus, this also focuses the inner window if it exists. */
void set_focus_frame(Frame *frame);

/* Focus @window and the frame it is in. */
void set_focus_window_with_frame(Window *window);

/* Get the frame above the given one that has no parent. */
Frame *get_root_frame(Frame *frame);

#endif
