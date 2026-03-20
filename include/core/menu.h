#ifndef MENU_H
#define MENU_H

#include "font.h"
#include "utility/list.h"
#include "x11/synchronize.h"

/* user menu window */
typedef struct menu {
    /* the X correspondence */
    XReference reference;
    /* Xft drawing context */
    XftDraw *xft_draw;
    /* the typed string */
    LIST(FcChar32, string);
    /* the currently selected window index */
    unsigned selected;
    /* the currently scrolled amount in pixels */
    int scrolling;
    /* menu items */
    struct menu_item {
        /* string representation of the item */
        utf8_t *string;
        /* text objext */
        Text *text;
        /* the fuzzy match score of this item */
        unsigned score;
        /* additional score for often used entries */
        unsigned priority;
        /* extra context data for specific menus */
        void *data;
    } *items;
    /* the number of items */
    unsigned number_of_items;
    /* the total width and height the items require */
    unsigned width;
    unsigned height;
} Menu;

/* the menu to choose applications from */
extern Menu *application_chooser;

/* the menu to choose windows from */
extern Menu *window_chooser;

/* Create a menu for showing items the user can select from. */
Menu *create_menu(void);

/* Handle an incoming X event for all menu windows. */
void handle_menu_event(XEvent *event);

/* Choose an application to run.
 *
 * If an application is being chosen already, this will cancel it.
 *
 * This grabs the keyboard.
 *
 * @return ERROR if the application chooser could not be created.
 */
int show_application_chooser(void);

/* Choose a window to show and focus.
 *
 * If a window is being chosen already, this will cancel it.
 *
 * @return ERROR if the window chooser could not be created.
 */
int show_window_chooser(void);

#endif
