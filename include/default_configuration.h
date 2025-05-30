#ifndef DEFAULT_CONFIGURATION_H
#define DEFAULT_CONFIGURATION_H

#include "configuration.h"

/* Load the default values into the configuration. */
void load_default_configuration(void);

/* Puts the mousebindings of the default configuration into @configuration
 * without overwriting any mousebindings.
 */
void merge_with_default_button_bindings(struct configuration *configuration);

/* Puts the keybindings of the default configuration into @configuration without
 * overwriting any keybindings.
 */
void merge_with_default_key_bindings(struct configuration *configuration);

#endif
