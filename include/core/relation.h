#ifndef RELATION_H
#define RELATION_H

/**
 * Relations are actions that run when a window class/instance matches the
 * relation.
 */

#include <utility/attributes.h>
#include <utility/types.h>

#include "bits/action_block.h"
#include "bits/window.h"
#include "bits/window_pattern.h"

/* relation between class/instance and actions */
struct window_relation {
    /* unique name of the relation */
    utf8_t *name;
    /* if the relation should only be executed once a window is created */
    bool is_once;
    /* the pattern the window must match */
    WindowPattern pattern;
    /* the actions to execute */
    ActionBlock *actions;
};

/* Clear the memory occupied by the window relation. */
void clear_window_relation(struct window_relation *relation);

/* Duplicate a relation deeply into itself. */
void duplicate_window_relation(struct window_relation *relation);

/* Remove the currently running window relation. */
void remove_current_window_relation(void);

/* Set a window relation from instance/class name to actions.
 *
 * All memory from the given relation is duplicated.
 */
void set_window_relation(const struct window_relation *relation);

/* Remove all window relations whose name matches given pattern. */
void remove_window_relations_by_pattern(const utf8_t *pattern);

/* Unset all currently set window relations. */
void unset_window_relations(void);

/* Run the actions related to given window.
 *
 * @is_once_included controls whether windows should
 *
 * @return if any once relation ran.
 */
bool run_window_relations(FcWindow *window, bool is_once_included);

#endif
