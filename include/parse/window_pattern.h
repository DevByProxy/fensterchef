#ifndef PARSE__WINDOW_PATTERN_H
#define PARSE__WINDOW_PATTERN_H

/**
 * Parse a window pattern.
 *
 * title STRING
 * class STRING
 * instance STRING
 *
 * The above may be combined using commas in between.
 * For example:
 * title *App, class xterm
 *
 * Legacy syntax:
 * STRING
 * STRING, STRING
 */

#include "bits/window_pattern.h"
#include "parse/parse.h"

/* Continue parsing a window pattern.
 *
 * A keyword (string) must have been read into @parser.
 */
void continue_parsing_window_pattern(Parser *parser,
        WindowPattern *pattern);

#endif
