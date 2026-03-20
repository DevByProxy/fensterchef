#include <utility/utility.h>

#include "parse/input.h"
#include "parse/utility.h"
#include "parse/window_pattern.h"

/* Read an optional ", CLASS" after reading a string. */
static void read_legacy_class_string(Parser *parser,
        WindowPattern *pattern)
{
    utf8_t *first_pattern;

    first_pattern = xstrdup(parser->string);

    skip_blanks(parser);
    if (peek_stream_character(parser) == ',') {
        /* skip ',' */
        (void) get_stream_character(parser);

        if (read_string(parser) != OK) {
            emit_parse_error(parser,
                    "expected class name");
            pattern->class = NULL;
        } else {
            pattern->class = xstrdup(parser->string);
        }

        pattern->instance = first_pattern;
    } else {
        pattern->class = first_pattern;
    }
}

/* Continue parsing a window pattern. */
void continue_parsing_window_pattern(Parser *parser,
        WindowPattern *pattern)
{
    ZERO(pattern, 1);

    /* start by matching all modes */
    pattern->mode = WINDOW_MODE_MAX;

    while (true) {
        if (!parser->is_string_quoted &&
                (strcmp(parser->string, "mode") == 0 ||
                    strcmp(parser->string, "name") == 0 ||
                    strcmp(parser->string, "title") == 0 ||
                    strcmp(parser->string, "instance") == 0 ||
                    strcmp(parser->string, "class") == 0)) {
            char indicator;
            char **target;

            indicator = parser->string[0];
            if (read_string(parser) != OK) {
                emit_parse_error(parser, "expected string after %s",
                        parser->string);
                break;
            }

            if (indicator == 'm') {
                if (strcmp(parser->string, "tiling") == 0) {
                    pattern->mode = WINDOW_MODE_TILING;
                } else if (strcmp(parser->string, "floating") == 0) {
                    pattern->mode = WINDOW_MODE_FLOATING;
                } else if (strcmp(parser->string, "fullscreen") == 0) {
                    pattern->mode = WINDOW_MODE_FULLSCREEN;
                } else if (strcmp(parser->string, "dock") == 0) {
                    pattern->mode = WINDOW_MODE_DOCK;
                } else if (strcmp(parser->string, "desktop") == 0) {
                    pattern->mode = WINDOW_MODE_DESKTOP;
                } else {
                    emit_parse_error(parser, "mode %s does not exist",
                            parser->string);
                }
            } else {
                switch (indicator) {
                case 'n':
                case 't': target = &pattern->name; break;
                case 'i': target = &pattern->instance; break;
                case 'c': target = &pattern->class; break;
                default:
                    UNREACHABLE;
                }
                if (*target != NULL) {
                    emit_parse_error(parser, "property already specified");
                } else {
                    *target = xstrdup(parser->string);
                }
            }
        } else {
            if (pattern->instance != NULL ||
                    pattern->class != NULL) {
                free(pattern->instance);
                free(pattern->class);
                emit_parse_error(parser, "mixing legacy syntax with newer");
            }
            /* legacy syntax */
            read_legacy_class_string(parser, pattern);
            break;
        }

        if (peek_stream_character(parser) != ',') {
            break;
        }

        /* skip ',' */
        (void) get_stream_character(parser);

        if (read_string(parser) != OK) {
            emit_parse_error(parser, "expected one of title|name|class");
            break;
        }
    }

    if (pattern->name == NULL) {
        pattern->name = xstrdup("*");
    }
    if (pattern->instance == NULL) {
        pattern->instance = xstrdup("*");
    }
    if (pattern->class == NULL) {
        pattern->class = xstrdup("*");
    }
}
