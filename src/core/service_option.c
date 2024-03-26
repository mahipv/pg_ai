#include "service_option.h"

#include <postgres.h>

/*
 * Allocate a new ServiceOption node.
 */
static ServiceOption *get_new_node()
{
	return ((ServiceOption *)palloc0(sizeof(ServiceOption)));
}

/*
 * Function to define a service specific option and add it to the list of
 * options at the front.
 */
void define_new_option(ServiceOption **option_list, const char *name,
					   char *description, const uint32_t flags,
					   char *storage_ptr, const size_t max_storage_size)
{
	ServiceOption *header = *option_list;
	ServiceOption *new_node = get_new_node();

	strncpy(new_node->name, name, OPTION_NAME_LEN);
	strncpy(new_node->description, description, OPTION_VALUE_LEN);
	new_node->flags = flags;
	new_node->value_ptr = NULL;

	/* insert the node in the list */
	*option_list = new_node;
	if (header)
	{
		new_node->next = header;
		header->prev = new_node;
	}

	/*
	 * If a storage ptr is passed reuse the same (optimization to avoid
	 * duplicate copies) otherwise allocate a new memory.
	 */
	if (storage_ptr)
	{
		new_node->value_ptr = storage_ptr;
		new_node->max_len = max_storage_size;
	}
	else
	{
		new_node->value_ptr = palloc0(OPTION_VALUE_LEN);
		new_node->max_len = OPTION_VALUE_LEN;
	}

	/* initialize for a concat */
	new_node->value_ptr[0] = '\0';
}

/*
 * Set the value for a particular option. If value_ptr is NULL, a new memory
 * is allocated and the value is copied to it. Otherwise, the value_ptr is
 * used to store the value. data_size is used to check the max length of the
 * value_ptr.
 */
int set_option_value(ServiceOption *list, const char *name, const char *value,
					 bool concat)
{
	ServiceOption *node = list;
	bool found = false;
	size_t len;

	while (node && !found)
	{
		if (!strcmp(name, node->name))
		{
			len = strlen(value);

			if ((concat) && (node->current_len + len > node->max_len))
				ereport(ERROR,
						(errmsg("Value for option %s is too long", name)));

			if (concat)
			{
				strcat(node->value_ptr, " ");
				strncat(node->value_ptr, value, len);
				node->current_len += len + 1;
			}
			else
			{
				strncpy(node->value_ptr, value, len);
				node->current_len = len;
			}

			node->flags |= OPTION_FLAG_IS_SET;
			found = true;
			break;
		}
		node = node->next;
	}

	return !found;
}

/*
 * Get the value for a particular option, NULL if the option is not found.
 */
char *get_option_value(ServiceOption *list, const char *name)
{
	ServiceOption *node = list;

	while (node)
	{
		if (!strcmp(name, node->name))
			return node->value_ptr;
		node = node->next;
	}

	return NULL;
}

/*
 * Get the length of the value for a particular option, -1 if the option is
 * not found.
 */
int get_option_value_length(ServiceOption *list, const char *name)
{
	ServiceOption *node = list;

	while (node)
	{
		if (!strcmp(name, node->name))
			return node->current_len;
		node = node->next;
	}

	return -1;
}

/*
 * Get the max length of the value for a particular option, -1 if the option is
 * not found.
 */
int get_option_value_max_length(ServiceOption *list, const char *name)
{
	ServiceOption *node = list;

	while (node)
	{
		if (!strcmp(name, node->name))
			return node->max_len;
		node = node->next;
	}

	return -1;
}

/*
 * Get the option node for a particular option, NULL if the option is not found.
 */
ServiceOption *get_option(ServiceOption *list, const char *name)
{
	ServiceOption *node = list;

	while (node)
	{
		if (!strcmp(name, node->name))
			return node;
		node = node->next;
	}

	return NULL;
}

/*
 * Print the service options to the console or to a text buffer. If print_value
 * is true, the value of the option is printed, otherwise the description is
 * printed.
 */
void print_service_options(ServiceOption *list, bool print_value, char *text,
						   size_t max_len)
{
	ServiceOption *last_node = list;
	char option_info[OPTION_NAME_LEN + OPTION_VALUE_LEN + 8];

	/* options are added to the front display from last to get the same order */
	while (last_node && last_node->next)
		last_node = last_node->next;

	while (last_node)
	{
		/* TODO print value by default if DEBUG */
		if (print_value)
			sprintf(option_info, "\n%s: %s", last_node->name,
					last_node->value_ptr);
		else
			sprintf(option_info, "\n%s: %s", last_node->name,
					last_node->description);

		/* print all options in case of console */
		if (!text)
			ereport(INFO, (errmsg("%s", option_info)));
		else if (last_node->flags & OPTION_FLAG_HELP_DISPLAY)
			strncat(text, option_info, max_len);

		last_node = last_node->prev;
	}
}
