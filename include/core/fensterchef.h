#ifndef FENSTERCHEF_H
#define FENSTERCHEF_H

#include <stdbool.h>

#include <X11/X.h> /* Window */

#include "utility/attributes.h"

#define FENSTERCHEF_NAME "fensterchef"

#define FENSTERCHEF_VERSION "2.1-3"

#define FENSTERCHEF_CONFIGURATION "fensterchef/wm"

#define FENSTERCHEF_RESTORE_FILE "LAYOUT_DUMP"

/* the path of the fensterchef executable */
extern char *Fensterchef_executable;

/* the home directory */
extern char *Fensterchef_home;

/* the path of the configuration file as specified by the user */
extern _Nullable char *Fensterchef_configuration;

/* true while the window manager is running */
extern bool Fensterchef_is_running;

/* true if the user requested fensterchef to update by restarting */
extern bool Fensterchef_is_update_requested;

/* Allocate a path to a state file.
 *
 * Use `free()` on the return value.
 *
 * @file_name is the name of the file to append to the directory.
 */
char *allocate_state_path(const char *file_name);

/* Run fensterchef again and replace the current process.
 *
 * @return ERROR if the layout could not be dumped or the new executable can not
 *         be opened.
 */
int update_fensterchef(void);

/* Spawn a window that has the `FENSTERCHEF_COMMAND` property.
 *
 * This will exit the program.
 */
void run_external_command(const char *command);

/* Check for an external fensterchef command on a window. */
void check_for_external_command(Window window);

/* Output the frames and windows into a file as textual output.
 *
 * @return ERROR if the file could not be opened, OK otherwise.
 */
int dump_frames_and_windows(const char *file_path);

enum dump_parse_error {
    /* all went fine */
    DUMP_SUCCESS,
    /* file IO error, see `errno` */
    DUMP_IO_ERROR,
    /* the file has no content */
    DUMP_EMPTY_FILE,
    /* the global section [Global] is missing or invalid */
    DUMP_GLOBAL_SECTION_ERROR,
    /* [Frames] */
    DUMP_FRAMES_SECTION_ERROR,
    /* [Stash] */
    DUMP_STASH_SECTION_ERROR,
    /* [Windows] */
    DUMP_WINDOWS_SECTION_ERROR,
};

/* Restore the frames and windows from a file.
 *
 * @return one of the above constants.
 */
enum dump_parse_error restore_frames_and_windows(const char *file_path);

/* Get a string representation of the error. */
const char *get_string_of_dump_parse_error(enum dump_parse_error error);

/* Close the connection to the X server and exit the program with given exit
 * code.
 */
void quit_fensterchef(int exit_code);

#endif
