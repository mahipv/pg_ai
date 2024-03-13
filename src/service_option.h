#ifndef _SERVICE_OPTION_H_
#define _SERVICE_OPTION_H

#include <stdbool.h>

#define OPTION_NAME_LEN 255
#define OPTION_VALUE_LEN 1*1024

typedef struct ServiceOption
{
	char		name[OPTION_NAME_LEN];
	char		value[OPTION_VALUE_LEN];
	char		description[OPTION_VALUE_LEN];
	int			guc_option;
	int			required;
	bool		is_set;
	bool 		help_display;
	struct ServiceOption *next;
	struct ServiceOption *prev;
}			ServiceOption;

void		define_new_option(ServiceOption * *option_list, const char *name,
							  char *description, const int guc_option,
							  const int required, bool help_display);
int			set_option_value(ServiceOption * list, const char *name, const char *value);
int			get_option_value_copy(ServiceOption * list, const char *name, char *value);
char	   *get_option_value(ServiceOption * list, const char *name);
void		print_service_options(ServiceOption * list, int print_value);
void		delete_list(ServiceOption * list);

#endif							/* _SERVICE_OPTION_H */
