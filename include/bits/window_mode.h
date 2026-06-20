#ifndef WINDOW_MODE_H
#define WINDOW_MODE_H

typedef enum window_mode {
    WINDOW_MODE_TILING,
    WINDOW_MODE_FLOATING,
    WINDOW_MODE_FULLSCREEN,
    WINDOW_MODE_DOCK,
    WINDOW_MODE_DESKTOP,
    WINDOW_MODE_MAX
} window_mode_t;

#endif
