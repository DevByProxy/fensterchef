#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <utility/list.h>

#include <X11/Xatom.h>

#include "fensterchef.h"
#include "font.h"
#include "frame.h"
#include "log.h"
#include "parse/parse.h"
#include "window.h"
#include "x11/display.h"

/* the path of the fensterchef executable */
char *Fensterchef_executable;

/* the home directory */
char *Fensterchef_home;

/* the user specified path for the configuration file */
char *Fensterchef_configuration;

/* true while the window manager is running */
bool Fensterchef_is_running;

/* true if the user requested fensterchef to update by restarting */
bool Fensterchef_is_update_requested;

/* Allocate a path to a state file. */
char *allocate_state_path(const char *file_name)
{
    LIST(char, path);
    const char *xdg_state_home;
    size_t file_name_length;

    LIST_INITIALIZE(path, 256);

    xdg_state_home = getenv("XDG_STATE_HOME");
    if (xdg_state_home == NULL) {
        LIST_APPEND(path, Fensterchef_home, strlen(Fensterchef_home));
        LIST_APPEND(path, "/.local/state", strlen("/.local/state"));
    } else {
        LIST_APPEND(path, xdg_state_home, strlen(xdg_state_home));
    }

    LIST_APPEND_VALUE(path, '\0');
    path_length--;
    if (mkdir(path, 0755) == -1 && errno != EEXIST) {
        free(path);
        return NULL;
    }

    LIST_APPEND(path, "/fensterchef", strlen("/fensterchef"));
    LIST_APPEND_VALUE(path, '\0');
    path_length--;
    if (mkdir(path, 0755) == -1 && errno != EEXIST) {
        free(path);
        return NULL;
    }

    LIST_APPEND_VALUE(path, '/');
    /* +1 for the nul-terminator */
    file_name_length = strlen(file_name) + 1;
    LIST_GROW(path, path_length + file_name_length);
    LIST_APPEND(path, file_name, file_name_length);

    return path;
}

/* Run fensterchef again and replace the current process. */
int update_fensterchef(void)
{
    char *path;
    char *argv[7];

    path = allocate_state_path(FENSTERCHEF_RESTORE_FILE);
    if (path == NULL) {
        LOG_ERROR("could not create the state directory: %s\n",
                strerror(errno));
        return ERROR;
    }

    if (dump_frames_and_windows(path) != OK) {
        LOG_ERROR("could not write layout to %s: %s\n",
                path, strerror(errno));
        free(path);
        return ERROR;
    }

    free(path);

    /* say goodbye to the X server before coming back */
    XCloseDisplay(display);

    argv[0] = Fensterchef_executable;
    argv[1] = "--verbosity";
    switch (log_severity) {
    case LOG_SEVERITY_ALL: argv[2] = "all"; break;
    case LOG_SEVERITY_INFO: argv[2] = "info"; break;
    case LOG_SEVERITY_ERROR: argv[2] = "error"; break;
    case LOG_SEVERITY_NOTHING: argv[2] = "nothing"; break;
    }
    argv[3] = "--restore-layout";
    if (Fensterchef_configuration == NULL) {
        argv[4] = NULL;
    } else {
        argv[4] = "--config";
        argv[5] = Fensterchef_configuration;
        argv[6] = NULL;
    }

    LOG_DEBUG("starting fensterchef again: %s %s %s %s\n",
            argv[0], argv[1], argv[2], argv[3]);

    (void) execvp(argv[0], argv);

    LOG_ERROR("could not start the new executable at %s: %s\n",
            Fensterchef_executable, strerror(errno));

    /* whoops */
    exit(EXIT_FAILURE);
}

/* Spawn a window that has the `FENSTERCHEF_COMMAND` property. */
void run_external_command(const char *command)
{
    Display *display;

    XSetWindowAttributes attributes;
    Window window;

    Atom command_atom;

    XEvent event;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "fensterchef command: "
                    "could not connect to the X server\n");
        exit(EXIT_FAILURE);
    }

    attributes.event_mask = PropertyChangeMask;
    attributes.override_redirect = True;
    window = XCreateWindow(display, DefaultRootWindow(display), -1, -1, 1, 1, 0,
            CopyFromParent, InputOnly, (Visual*) CopyFromParent,
            CWEventMask | CWOverrideRedirect, &attributes);

    command_atom = XInternAtom(display, "FENSTERCHEF_COMMAND", False);

    XChangeProperty(display, window, command_atom, XA_STRING, 8,
            PropModeReplace, (unsigned char*) command, strlen(command));

    XMapWindow(display, window);

    fprintf(stderr, "fensterchef command: command was dispatched, "
                "waiting until execution...\n");

    /* wait until the property gets removed */
    while (true) {
        XNextEvent(display, &event);
        if (event.type == PropertyNotify) {
            /* if the property gets removed, it means the command was executed
             */
            if (event.xproperty.atom == command_atom &&
                    event.xproperty.state == PropertyDelete) {
                break;
            }
        }
    }

    XCloseDisplay(display);
    exit(EXIT_SUCCESS);
}

/* Check for an external fensterchef command on a window. */
void check_for_external_command(Window window)
{
    utf8_t *command;
    Parser *parser;

    /* check if the window holds a command */
    command = get_fensterchef_command_property(window);
    if (command != NULL) {
        LOG("window %#lx has command: %s\n",
                window, command);

        /* signal to the window that we are about to execute its command and
         * it can go now
         */
        (void) XDeleteProperty(display, window, ATOM(FENSTERCHEF_COMMAND));

        parser = create_string_parser(command);
        (void) parse_and_run_actions(parser);
        destroy_parser(parser);

        free(command);
    }
}

/* Close the display to the X server and exit the program with given exit
 * code.
 */
void quit_fensterchef(int exit_code)
{
    LOG("quitting fensterchef with exit code: %d\n", exit_code);
    /* when debugging, this avoids ugly messages from the sanitizer */
#ifdef DEBUG
    free_font_list();
#endif

    XCloseDisplay(display);
    exit(exit_code);
}

/******************************
 * Dumping the current layout *
 ******************************/

/* Dump a frame into a file. */
static void dump_frame(Frame *frame, unsigned indentation, FILE *file)
{
    for (unsigned i = 0; i < indentation; i++) {
        fputc(' ', file);
    }
    fprintf(file, "%p %u ",
            (void*) frame, frame->number);
    fprintf(file, "%d %d %u %u ",
            frame->x, frame->y, frame->width, frame->height);
    fprintf(file, "%p ",
            (void*) frame->window);
    fprintf(file, "%u/%u",
            frame->ratio.numerator, frame->ratio.denominator);
    switch (frame->split_direction) {
    case FRAME_SPLIT_HORIZONTALLY:
        fputc('H', file);
        break;
    case FRAME_SPLIT_VERTICALLY:
        fputc('V', file);
        break;
    }
    fputc('\n', file);
    if (frame->left != NULL) {
        dump_frame(frame->left, indentation + 1, file);
        dump_frame(frame->right, indentation + 1, file);
    }
}

/* Output the frames and windows into a file as textual output. */
int dump_frames_and_windows(const char *file_path)
{
    FILE *file;

    file = fopen(file_path, "w");
    if (file == NULL) {
        return ERROR;
    }

    fputs("[Global]\n", file);
    fprintf(file, "%p %p\n%p\n%p %p\n",
            (void*) Frame_focus, (void*) Window_focus,
            (void*) Window_oldest,
            (void*) Window_bottom, (void*) Window_top);

    fputs("[Frames]\n", file);
    for (Monitor *monitor = Monitor_first;
            monitor != NULL;
            monitor = monitor->next) {
        dump_frame(monitor->frame, 0, file);
    }

    fputs("[Stash]\n", file);
    for (Frame *frame = Frame_last_stashed;
            frame != NULL;
            frame = frame->previous_stashed) {
        dump_frame(frame, 0, file);
    }

    fputs("[Windows]\n", file);
    for (FcWindow *window = Window_first;
            window != NULL;
            window = window->next) {
        fprintf(file, "%s\n%p %#lx %u %c%c",
                window->properties.name == NULL ? "" : window->properties.name,
                (void*) window, window->reference.id, window->number,
                window->state.is_visible ? 'V' : 'I',
                window->state.is_auto_focus_allowed ? 'A' : 'N');

        switch (window->state.previous_mode) {
        case WINDOW_MODE_TILING: fputc('T', file); break;
        case WINDOW_MODE_FLOATING: fputc('O', file); break;
        case WINDOW_MODE_FULLSCREEN: fputc('F', file); break;
        case WINDOW_MODE_DOCK: fputc('D', file); break;
        case WINDOW_MODE_DESKTOP: fputc('P', file); break;
        case WINDOW_MODE_MAX: fputc('N', file); break;
        }

        switch (window->state.mode) {
        case WINDOW_MODE_TILING: fputc('T', file); break;
        case WINDOW_MODE_FLOATING: fputc('O', file); break;
        case WINDOW_MODE_FULLSCREEN: fputc('F', file); break;
        case WINDOW_MODE_DOCK: fputc('D', file); break;
        case WINDOW_MODE_DESKTOP: fputc('P', file); break;
        case WINDOW_MODE_MAX: fputc('N', file); break;
        }

        fprintf(file, " %d %d %u %u %u %" PRIu32 " %" PRIu32 " %" PRIu32
                      " %d %d %u %u %p %p %p\n",
                window->x, window->y, window->width, window->height,
                window->border_size, window->border_color,
                window->border_color_active, window->border_color_focus,
                window->floating.x, window->floating.y,
                window->floating.width, window->floating.height,
                (void*) window->newer,
                (void*) window->below, (void*) window->above);
    }

    fclose(file);
    return OK;
}

/*******************************
 * Restoring a previous layout *
 *******************************/

struct file_frame;
typedef struct file_frame FileFrame;

struct file_frame {
    /* the pointer this frame used to have */
    void *pointer;
    /* the pointer of the window */
    void *window;
    /* position and size */
    int x, y;
    unsigned width, height;
    /* ratio of the children */
    Ratio ratio;
    /* split direction */
    frame_split_direction_t split_direction;
    /* the id */
    unsigned number;
    /* the parent */
    FileFrame *parent;
    /* the children */
    FileFrame *left, *right;
    /* a next frame */
    FileFrame *next;
};

struct file_window;
typedef struct file_window FileWindow;

struct file_window {
    /* get the real window */
    FcWindow *real_window;
    /* If a visible frame took this window for itself.
     * This is used after the parsing.
     */
    bool is_occupied;
    /* the pointer this window used to have */
    void *pointer;
    /* the X window id */
    Window id;
    /* the number of the window */
    unsigned number;
    /* the window state */
    window_mode_t previous_mode, mode;
    bool is_visible;
    bool is_auto_focus_allowed;
    /* position and size */
    int x, y;
    unsigned width, height;
    /* border */
    unsigned border_size;
    uint32_t border_color, border_color_active, border_color_focus;
    /* floating position */
    Rectangle floating;
    /* related windows */
    void *newer, *below, *above;
    FileWindow *next;
};

struct dump_parser {
    /* the file to read lines from */
    FILE *file;
    /* the line data */
    LIST(char, line);

    /* the first frame read from the stash or frames section */
    FileFrame *first_read_frame;
    /* the current frame being read */
    FileFrame *current_frame;
    /* the depth of the last child frame */
    int previous_depth;

    /* the global data */
    void *frame_focus, *window_focus;
    void *window_oldest;
    void *window_bottom, *window_top;
    /* the first frame of the frames section */
    FileFrame *first_frame;
    /* the first frame of the stash section */
    FileFrame *first_stash;
    /* the first window of the windows section */
    FileWindow *first_window;
    /* the current window being read */
    FileWindow *current_window;
};

/* Read the next line from the file.
 *
 * @return NULL when the end of the file has been reached.
 */
static char *read_line(struct dump_parser *reader)
{
    char buffer[1024];
    char *string;
    char *end;
    size_t length;
    bool is_eof = true;

    reader->line_length = 0;
    do {
        string = fgets(buffer, sizeof(buffer), reader->file);
        if (string == NULL) {
            break;
        }

        end = strchr(string, '\n');
        if (end == NULL) {
            length = strlen(string);
        } else {
            length = end - string;
        }
        LIST_APPEND(reader->line, string, length);
        is_eof = false;
    } while (end == NULL);

    LIST_APPEND_VALUE(reader->line, '\0');

    if (is_eof) {
        return NULL;
    }

    return reader->line;
}

/* Clear all memory associated to the frame including children. */
static void clear_file_frame(_Nullable FileFrame *frame)
{
    if (frame == NULL) {
        return;
    }

    clear_file_frame(frame->left);
    clear_file_frame(frame->right);

    free(frame);
}

/* Close the file and free all memory. */
static void clear_reader(struct dump_parser *reader)
{
    for (FileWindow *window = reader->first_window, *window_next;
            window != NULL;
            window = window_next) {
        window_next = window->next;
        free(window);
    }

    clear_file_frame(reader->first_read_frame);

    clear_file_frame(reader->first_frame);
    clear_file_frame(reader->first_stash);

    fclose(reader->file);
    free(reader->line);
}

/* Parse the global section. */
static int scan_global_section(struct dump_parser *reader)
{
    char *line;

    line = read_line(reader);
    if (sscanf(line, "%p%p",
                &reader->frame_focus, &reader->window_focus) != 2) {
        return ERROR;
    }

    line = read_line(reader);
    if (line == NULL) {
        return ERROR;
    }
    if (sscanf(line, "%p", &reader->window_oldest) != 1) {
        return ERROR;
    }

    line = read_line(reader);
    if (line == NULL) {
        return ERROR;
    }
    if (sscanf(line, "%p%p",
                &reader->window_bottom, &reader->window_top) != 2) {
        return ERROR;
    }

    /* read the next line */
    line = read_line(reader);
    if (line == NULL) {
        return ERROR;
    }
    return OK;
}

/* Parse the frames section. */
static int scan_frames_section_entry(struct dump_parser *reader)
{
    char *line;
    int depth = 0;
    char character;
    FileFrame *frame;

    line = reader->line;
    while (line[0] == ' ') {
        depth++;
        line++;
    }

    frame = reader->current_frame;

    /* move back out */
    while (depth < reader->previous_depth) {
        if (frame->left != NULL &&
                frame->right == NULL) {
            /* moving out too early */
            return ERROR;
        }
        frame = frame->parent;
        reader->previous_depth--;
    }

    if (reader->first_read_frame == NULL) {
        if (depth > 0) {
            /* the root needs depth 0 */
            return ERROR;
        }
        ALLOCATE_ZERO(reader->first_read_frame, 1);
        frame = reader->first_read_frame;
    } else if (depth - 1 == reader->previous_depth) {
        /* moving into a deeper child */
        ALLOCATE_ZERO(frame->left, 1);
        frame->left->parent = frame;
        frame = frame->left;
    } else if (depth == reader->previous_depth) {
        /* moving to a neighbord */
        if (frame->parent == NULL) {
            ALLOCATE_ZERO(frame->next, 1);
            frame = frame->next;
        } else if (frame->parent->right == NULL) {
            ALLOCATE_ZERO(frame->parent->right, 1);
            frame->parent->right->parent = frame->parent;
            frame = frame->parent->right;
        } else {
            /* too many children */
            return ERROR;
        }
    } else if (depth > reader->previous_depth) {
        /* the depth makes no sense */
        return ERROR;
    }

    reader->previous_depth = depth;

    if (sscanf(line, "%p %u %d %d %u %u %p %u/%u %c\n",
            &frame->pointer, &frame->number,
            &frame->x, &frame->y,
            &frame->width, &frame->height,
            &frame->window,
            &frame->ratio.numerator, &frame->ratio.denominator,
            &character) != 10) {
        /* invalid format */
        return ERROR;
    }

    if (character == 'H') {
        frame->split_direction = FRAME_SPLIT_HORIZONTALLY;
    } else if (character == 'V') {
        frame->split_direction = FRAME_SPLIT_VERTICALLY;
    }

    reader->current_frame = frame;

    return OK;
}

/* Parse a windows section entry. */
static int scan_windows_section_entry(struct dump_parser *reader)
{
    char *line;
    char is_visible, auto_focus, previous_mode, mode;
    FileWindow *window;

    /* skip to the next line because we do not need the window name */
    line = read_line(reader);
    if (line == NULL) {
        return ERROR;
    }

    if (reader->first_window == NULL) {
        ALLOCATE_ZERO(reader->first_window, 1);
        window = reader->first_window;
    } else {
        ALLOCATE_ZERO(reader->current_window->next, 1);
        window = reader->current_window->next;
    }

    if (sscanf(line, "%p %lx %u %c%c%c%c %d %d %u %u %u %" PRIu32
                     " %" PRIu32 " %" PRIu32 " %d %d %u %u %p %p %p",
            &window->pointer, &window->id, &window->number,
            &is_visible, &auto_focus, &previous_mode, &mode,
            &window->x, &window->y, &window->width, &window->height,
            &window->border_size, &window->border_color,
            &window->border_color_active, &window->border_color_focus,
            &window->floating.x, &window->floating.y,
            &window->floating.width, &window->floating.height,
            &window->newer, &window->below, &window->above) != 22) {
        return ERROR;
    }

    window->is_visible = is_visible == 'V';
    window->is_auto_focus_allowed = auto_focus == 'A';

    switch (previous_mode) {
    case 'T': window->previous_mode = WINDOW_MODE_TILING; break;
    case 'O': window->previous_mode = WINDOW_MODE_FLOATING; break;
    case 'F': window->previous_mode = WINDOW_MODE_FULLSCREEN; break;
    case 'D': window->previous_mode = WINDOW_MODE_DOCK; break;
    case 'P': window->previous_mode = WINDOW_MODE_DESKTOP; break;
    case 'N': window->previous_mode = WINDOW_MODE_MAX; break;
    }

    switch (mode) {
    case 'T': window->mode = WINDOW_MODE_TILING; break;
    case 'O': window->mode = WINDOW_MODE_FLOATING; break;
    case 'F': window->mode = WINDOW_MODE_FULLSCREEN; break;
    case 'D': window->mode = WINDOW_MODE_DOCK; break;
    case 'P': window->mode = WINDOW_MODE_DESKTOP; break;
    case 'N': window->mode = WINDOW_MODE_MAX; break;
    }

    reader->current_window = window;

    return OK;
}

/* Parse a section with multiple entries. */
static int scan_section_entries(struct dump_parser *reader,
        int (*scan_section_entry)(struct dump_parser *reader),
        bool is_last_section)
{
    char *line;

    while (line = read_line(reader), line != NULL) {
        if (!is_last_section && line[0] == '[') {
            break;
        }
        if (scan_section_entry(reader) != OK) {
            return ERROR;
        }
    }
    return OK;
}

/* Find the window that used to have given pointer. */
static FileWindow *find_window_by_pointer(const struct dump_parser *reader,
        const void *pointer)
{
    for (FileWindow *window = reader->first_window;
            window != NULL;
            window = window->next) {
        if (window->pointer == pointer) {
            return window;
        }
    }
    return NULL;
}

/* Extract given file frame into @frame.
 *
 * @is_visible controls whether the frame will be put on screen.
 */
static void extract_file_frame(const struct dump_parser *reader,
        FileFrame *file_frame, Frame *frame, bool is_visible)
{
    frame->ratio = file_frame->ratio;
    frame->split_direction = file_frame->split_direction;
    frame->number = file_frame->number;
    if (is_visible && file_frame->pointer == reader->frame_focus) {
        Frame_focus = frame;
    }
    if (file_frame->window != NULL) {
        FileWindow *window;

        window = find_window_by_pointer(reader, file_frame->window);
        if (window != NULL && window->real_window != NULL &&
                /* make sure no two visible frames get the same window */
                (!is_visible || !window->is_occupied)) {
            frame->window = window->real_window;
            if (is_visible) {
                reload_frame(frame);
                window->real_window->state.is_visible = true;
                window->is_occupied = true;
            } else {
                /* stashed windows need to be referenced */
                reference_window(window->real_window);
            }
        }
    /* we must check for both children in case the dump file is misformatted */
    } else if (file_frame->left != NULL && file_frame->right != NULL) {
        frame->left = create_frame();
        frame->right = create_frame();
        frame->left->parent = frame;
        frame->right->parent = frame;
        extract_file_frame(reader, file_frame->left, frame->left, is_visible);
        extract_file_frame(reader, file_frame->right, frame->right, is_visible);
    }
}

/* Restore the frames and windows from a file. */
enum dump_parse_error restore_frames_and_windows(const char *file_path)
{
    enum dump_parse_error result = DUMP_SUCCESS;
    struct dump_parser reader;
    Frame *old_focus;
    FileWindow *below, *window;

    ZERO(&reader, 1);

    reader.file = fopen(file_path, "r");
    if (reader.file == NULL) {
        return DUMP_IO_ERROR;
    }

    if (read_line(&reader) == NULL) {
        result = DUMP_EMPTY_FILE;
        goto finish;
    }

    if (strcmp(reader.line, "[Global]") != 0 ||
            scan_global_section(&reader) != OK) {
        result = DUMP_GLOBAL_SECTION_ERROR;
        goto finish;
    }

    if (strcmp(reader.line, "[Frames]") != 0 ||
            scan_section_entries(&reader,
                scan_frames_section_entry, false) != OK ||
            /* check if at least one frame was read */
            reader.first_read_frame == NULL) {
        result = DUMP_FRAMES_SECTION_ERROR;
        goto finish;
    }

    reader.first_frame = reader.first_read_frame;
    /* this needs to be reset to avoid double frees in case the
     * stash section fails to parse
     */
    reader.first_read_frame = NULL;
    /* reset these values for the stash section */
    reader.current_frame = NULL;
    reader.previous_depth = 0;

    if (strcmp(reader.line, "[Stash]") != 0 ||
            scan_section_entries(&reader,
                scan_frames_section_entry, false) != OK) {
        result = DUMP_STASH_SECTION_ERROR;
        goto finish;
    }

    reader.first_stash = reader.first_read_frame;
    /* set this to NULL so it does not get freed later when the
     * reader is cleared
     */
    reader.first_read_frame = NULL;

    if (strcmp(reader.line, "[Windows]") != 0 ||
            scan_section_entries(&reader,
                scan_windows_section_entry, true) != OK) {
        result = DUMP_WINDOWS_SECTION_ERROR;
        goto finish;
    }

    for (FileWindow *window = reader.first_window;
            window != NULL;
            window = window->next) {
        FcWindow *fc;

        fc = get_fensterchef_window(window->id);
        if (fc == NULL) {
            continue;
        }
        /* hide all windows and then show them later again */
        hide_window(fc);

        window->real_window = fc;

        set_window_size(fc, window->x, window->y,
                window->width, window->height);

        fc->border_size = window->border_size;
        fc->border_color = window->border_color;
        fc->border_color_active = window->border_color_active;
        fc->border_color_focus = window->border_color_focus;

        fc->floating = window->floating;

        set_window_mode(fc, window->mode);
        window->previous_mode = window->mode;

        set_window_number(fc, window->number);
    }

    old_focus = Frame_focus;
    reference_frame(Frame_focus);

    for (FileFrame *frame = reader.first_frame;
            frame != NULL;
            frame = frame->next) {
        Monitor *monitor;

        monitor = get_monitor_from_rectangle(frame->x, frame->y,
                frame->width, frame->height);
        if (monitor != NULL) {
            stash_frame(monitor->frame);
            extract_file_frame(&reader, frame, monitor->frame, true);
        } else {
            Frame *stash;

            stash = create_frame();
            extract_file_frame(&reader, frame, stash, false);
            link_frame_into_stash(stash);
        }
    }

    /* if the focus has not changed, make sure it points to a valid frame */
    if (Frame_focus == old_focus) {
        /* check if the frame got destroyed or stashed */
        if (old_focus->reference_count == 1 || Frame_last_stashed == NULL ||
                old_focus == Frame_last_stashed) {
            dereference_frame(old_focus);
            /* fall back to the main monitor frame */
            Frame_focus = Monitor_first->frame;
        }
    }

    /* clear the old stash to avoid the stash from filling up too much, the
     * stash should maybe be managed differently so that it does not fill up
     * like that, this is a quick fix for now TODO
     */
    clear_stash();
    /* if there were any stash frames defined, simply add then to the stash */
    if (reader.first_stash != NULL) {
        for (FileFrame *frame = reader.first_stash;
                frame != NULL;
                frame = frame->next) {
            Frame *stash;

            stash = create_frame();
            extract_file_frame(&reader, frame, stash, false);
            link_frame_into_stash(stash);
        }
    }

    below = NULL;
    window = find_window_by_pointer(&reader, reader.window_bottom);
    while (window != NULL) {
        while (window->real_window == NULL) {
            window = find_window_by_pointer(&reader, window->above);
            if (window == NULL) {
                break;
            }
        }

        if (window == NULL) {
            break;
        }

        if (below != NULL) {
            DOUBLY_RELINK_AFTER(Window_bottom, Window_top,
                    window->real_window, below->real_window,
                    below, above);
        }

        below = window;
        window = find_window_by_pointer(&reader, window->above);
    }

    /* show all windows that were defined to be visible */
    for (FileWindow *window = reader.first_window;
            window != NULL;
            window = window->next) {
        if (window->real_window != NULL) {
            if (window->is_visible) {
                show_window(window->real_window);
                if (window->pointer == reader.window_focus) {
                    set_focus_window(window->real_window);
                }
            }
        }
    }

    if (Window_focus == NULL) {
        set_focus_window(Frame_focus->window);
    }

finish:
    clear_reader(&reader);
    return result;
}

/* Get a string representation of the error. */
const char *get_string_of_dump_parse_error(enum dump_parse_error error)
{
    const char *strings[] = {
        [DUMP_SUCCESS] = "success",
        [DUMP_IO_ERROR] = "file error",
        [DUMP_EMPTY_FILE] = "success",
        [DUMP_GLOBAL_SECTION_ERROR] = "invalid global section",
        [DUMP_FRAMES_SECTION_ERROR] = "invalid frames section",
        [DUMP_STASH_SECTION_ERROR] = "invalid stash section",
        [DUMP_WINDOWS_SECTION_ERROR] = "invalid windows section",
    };
    return strings[error];
}
