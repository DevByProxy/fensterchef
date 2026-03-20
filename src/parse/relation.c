#include "core/window.h"
#include "parse/action.h"
#include "parse/input.h"
#include "parse/parse.h"
#include "parse/top.h"
#include "parse/utility.h"
#include "parse/window_pattern.h"

/* Parse the next assigment in the active stream. */
void continue_parsing_relation(Parser *parser,
        struct parse_action_block *block)
{
    struct window_relation relation;
    struct parse_action_block sub_block;
    struct action_block_item item;
    struct action_data data;

    if (read_string(parser) != OK) {
        emit_parse_error(parser, "expected relation name");
        return;
    }

    relation.name = xstrdup(parser->string);
    relation.is_once = false;

    if (read_string(parser) != OK) {
        emit_parse_error(parser, "expected window pattern or once keyword");
        free(relation.name);
        return;
    }

    if (!parser->is_string_quoted && strcmp(parser->string, "once") == 0) {
        relation.is_once = true;
        if (read_string(parser) != OK) {
            emit_parse_error(parser, "expected window pattern");
            free(relation.name);
            return;
        }
    }

    continue_parsing_window_pattern(parser, &relation.pattern);

    ZERO(&sub_block, 1);
    if (parse_top(parser, &sub_block) != OK) {
        emit_parse_error(parser,
                "expected actions after window pattern");
        free(relation.name);
        clear_parse_action_block(&sub_block);
        clear_window_pattern(&relation.pattern);
        return;
    }

    relation.actions = convert_parse_action_block(&sub_block);
    clear_parse_action_block(&sub_block);

    item.type = ACTION_RELATION;
    item.data_count = 1;
    LIST_APPEND_VALUE(block->items, item);

    data.flags = 0;
    data.type = ACTION_DATA_TYPE_RELATION;
    data.u.relation = relation;
    LIST_APPEND_VALUE(block->data, data);
}

/* Parse all after an `unrelate` keyword. */
void continue_parsing_unrelate(Parser *parser,
        struct parse_action_block *block)
{
    struct action_block_item item;
    struct action_data data;

    if (read_string(parser) != OK) {
        /* `unrelate` without a following string */
        item.type = ACTION_UNRELATE;
        item.data_count = 0;
        LIST_APPEND_VALUE(block->items, item);
    } else {
        item.type = ACTION_UNRELATE_S;
        item.data_count = 1;
        LIST_APPEND_VALUE(block->items, item);

        data.flags = 0;
        data.type = ACTION_DATA_TYPE_STRING;
        data.u.string = xstrdup(parser->string);
        LIST_APPEND_VALUE(block->data, data);
    }
}
