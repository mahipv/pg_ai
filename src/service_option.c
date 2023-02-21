#include "service_option.h"
#include <string.h>
#include "postgres.h"

/*
 * Allocate a new ServiceOption node.
 *
 * @return		pointer to the newly created node
 *
 */
static ServiceOption *
get_new_node()
{
	return((ServiceOption*)palloc0(sizeof(ServiceOption)));
}

/*
 * Function to define a service specific option and add it to the list of
 * options.
 *
 * @param[out]	option_list			add a new option to the list
 * @param[in]	name				name of the new option to be defined
 * @param[in]	description			brief description of the option showed in help
 * @param[in]	provider_option		non-zero if the option is  applicable to the provider
 *									and not service specific
 * @param[in]	required			non-zero if it is a required to set
 * @return		void
 *
 */
void
define_new_option(ServiceOption **option_list, const char *name, char *description,
				  const int provider_option, const int required)
{

	ServiceOption *last_node = *option_list;
	ServiceOption *new_node = get_new_node();

	strncpy(new_node->name, name, OPTION_NAME_LEN);
	strncpy(new_node->description, description, OPTION_VALUE_LEN);
	new_node->provider = provider_option;
	new_node->required = required;

	while(last_node && last_node->next != NULL)
		last_node = last_node->next;

	if (!last_node)
	{
		/* first node */
		*option_list = new_node;
	}
	else
	{
		last_node->next = new_node;
		new_node->prev = last_node;
	}
}

/*
 * Set the value for a particular option
 *
 * @param[in/out]	list			the option list
 * @param[in]		name			name of the new option to be set
 * @param[in]		value			the value that as to be set for the option
 * @return			zero on success, non-zero otherwise
 *
 */
int
set_option_value(ServiceOption *list, const char *name, const char *value)
{
	ServiceOption	*node = list;
	int				found = 0;

	while (node && !found)
	{
		if (!strcmp(name, node->name))
		{
			strncpy(node->value, value, OPTION_VALUE_LEN);
			found = 1;
			break;
		}
	}

	return !found;
}

/*
 * Get the value for a particular option
 *
 * @param[in]	list	the option list
 * @param[in]	name	name of the option
 * @param[out]	value	the value of the option copied to
 * @return			zero on success, non-zero otherwise
 *
 */
int
get_option_value_copy(ServiceOption *list, const char *name, char *value)
{
	ServiceOption	*node = list;
	int				found = 0;

	while (node && !found)
	{
		if (!strcmp(name, node->name))
		{
			strncpy(value, node->value, OPTION_VALUE_LEN);
			found = 1;
			break;
		}
	}

	return !found;
}

/*
 * Get the value for a particular option, NULL if the option is not found.
 *
 * @param[in]	list	the option list
 * @param[in]	name	name of the option
 * @return		pointer to value of the
 *
 */
char *
get_option_value(ServiceOption *list, const char *name)
{
	ServiceOption	*node = list;

	while (node)
	{
		if (!strcmp(name, node->name))
			return node->value;
		node = node->next;
	}

	return NULL;
}

/*
 * Get the value for a particular option, NULL if the option is not found.
 *
 * @param[in]	list			the option list
 * @param[in]	print_value		if non-zero name and value is printed(for debugging)
 *								otherwise prints name and description for help.
 * @return		void
 *
 */
void
print_service_options(ServiceOption *list, int print_value)
{
	ServiceOption	*option = list;

	while (option)
	{
		if (print_value)
			ereport(INFO,(errmsg("%s: %s\n",option->name, option->value)));
		else
			ereport(INFO,(errmsg("%s:%s\n",option->name, option->description)));
		option = option->next;
	}
}
