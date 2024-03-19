#ifndef _SERVICE_OPTION_H_
#define _SERVICE_OPTION_H_

#include <stdbool.h>
#include <stddef.h>

#define OPTION_NAME_LEN 255
#define OPTION_VALUE_LEN 1*1024

typedef struct ServiceOption
{
	char		name[OPTION_NAME_LEN];
	char		description[OPTION_VALUE_LEN];
	int			guc_option;
	int			required;
	bool		is_set;
	bool		help_display;
	char	   *value_ptr;
	struct ServiceOption *next;
	struct ServiceOption *prev;
}			ServiceOption;

void		define_new_option(ServiceOption * *option_list, const char *name,
							  char *description, const bool guc_option,
							  const bool required, const bool help_display);
int			set_option_value(ServiceOption * list, const char *name,
							 const char *value, char *value_ptr,
							 const size_t data_size);
char	   *get_option_value(ServiceOption * list, const char *name);
void		print_service_options(ServiceOption * list, bool print_value,
								  char *text, size_t max_len);

#endif							/* _SERVICE_OPTION_H */
