#ifndef WINDOW_PATTERN_H
#define WINDOW_PATTERN_H

#include <utility/types.h>

#include "bits/window_mode.h"

/* pattern a window can be matched against */
typedef struct window_pattern {
    /* the mode the window should match, this can be WINDOW_MODE_MAX in which
     * case no specific mode is required
     */
    window_mode_t mode;
    /* the pattern the name should match */
    utf8_t *name;
    /* the pattern the instance should match */
    utf8_t *instance;
    /* the pattern the class should match */
    utf8_t *class;
} WindowPattern;

#endif
