#include "relation.h"
#include "log.h"
#include "window.h"
#include "utility/list.h"

/* the window relations */
STATIC_LIST(struct window_relation, window_relations);

/* These two are needed to add/remove relations while relations are
 * running.  There would otherwise be numerous issues like base pointer changes,
 * index shifts.  These are all cleanly handled by these variables.
 *
 * See below in run_window_relations() on how they are used.
 */

/* the number of relations before starting to match relations */
static size_t old_window_relations_length;
/* the index of the currently running relation */
static size_t running_relation;

/* Clear the memory occupied by the window relation. */
void clear_window_relation(struct window_relation *relation)
{
    free(relation->name);
    clear_window_pattern(&relation->pattern);
    dereference_action_block(relation->actions);
}

/* Duplicate a relation deeply into itself. */
void duplicate_window_relation(struct window_relation *relation)
{
    relation->name = xstrdup(relation->name);
    duplicate_window_pattern(&relation->pattern);
    reference_action_block(relation->actions);
}

/* Remove the window relation at given index. */
static void remove_window_relation(size_t index)
{
    LOG_DEBUG("removing window relation %R\n",
            &window_relations[index]);

    clear_window_relation(&window_relations[index]);
    window_relations_length--;
    MOVE(&window_relations[index], &window_relations[index + 1],
            window_relations_length - index);

    if (index <= running_relation) {
        running_relation--;
    }

    /* tell `run_window_relations()` to check one less relation */
    old_window_relations_length--;
}

/* Remove the currently running window relation. */
void remove_current_window_relation(void)
{
    if (running_relation < window_relations_length) {
        remove_window_relation(running_relation);
    }
}

/* Set a relation from window instance/class name to actions. */
void set_window_relation(const struct window_relation *relation)
{
    size_t i = 0;

    for (; i < window_relations_length; i++) {
        struct window_relation *const existing_relation = &window_relations[i];
        if (strcmp(existing_relation->name, relation->name) == 0) {
            break;
        }
    }

    if (i == window_relations_length) {
        struct window_relation new_relation;

        LOG_DEBUG("adding window relation %R\n",
                relation);

        new_relation = *relation;
        duplicate_window_relation(&new_relation);
        LIST_APPEND_VALUE(window_relations, new_relation);
    } else {
        clear_window_relation(&window_relations[i]);
        window_relations[i] = *relation;
        duplicate_window_relation(&window_relations[i]);
    }
}

/* Remove all window relations whose name matches given pattern. */
void remove_window_relations_by_pattern(const utf8_t *pattern)
{
    for (size_t i = 0, n = window_relations_length; i < n; i++) {
        struct window_relation *const relation = &window_relations[i];
        if (matches_pattern(pattern, relation->name)) {
            remove_window_relation(i);
            i--;
            n--;
            break;
        }
    }
}

/* Unset all currently set window relations. */
void unset_window_relations(void)
{
    LOG_DEBUG("clearing all window relations\n");

    for (size_t i = 0; i < window_relations_length; i++) {
        clear_window_relation(&window_relations[i]);
    }

    LIST_CLEAR(window_relations);
}

/* Run all actions within an assocation after selecting @window. */
static inline void run_window_relation(FcWindow *window,
        struct window_relation *relation)
{
    LOG_DEBUG("running related actions: %A\n",
            relation->actions);

    Window_selected = window;
    run_action_block(relation->actions);
}

/* Run the actions related to given window. */
bool run_window_relations(FcWindow *window, bool is_once_included)
{
#ifdef DEBUG
    bool has_match = false;
#endif
    bool has_once_match = false;
    struct window_relation *relation;

    old_window_relations_length = window_relations_length;

    /* find all relations that match the window */
    for (size_t i = 0; i < old_window_relations_length; i++) {
        relation = &window_relations[i];
        if (!is_once_included && relation->is_once) {
            continue;
        }
        if (match_window_pattern(&relation->pattern, window) ==
                WINDOW_PATTERN_MATCH) {
            /* set back and forth in case the index changes because a
             * relation was removed
             */
            running_relation = i;
            run_window_relation(window, relation);
            i = running_relation;

            has_once_match |= relation->is_once;
#ifdef DEBUG
            has_match = true;
#endif
        }
    }

#ifdef DEBUG
    if (!has_match) {
        struct window_relation relation;

        relation.name = (char*) "_";
        relation.pattern.mode = window->state.mode;
        relation.pattern.name = window->properties.name;
        relation.pattern.instance = window->properties.class.res_name;
        relation.pattern.class = window->properties.class.res_class;
        LOG_DEBUG("no relation for %R\n",
                &relation);
    }
#endif

    return has_once_match;
}
