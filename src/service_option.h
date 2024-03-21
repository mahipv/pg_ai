#ifndef _SERVICE_OPTION_H_
#define _SERVICE_OPTION_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OPTION_NAME_LEN 255
#define OPTION_VALUE_LEN 1*1024

#define OPTION_FLAG_REQUIRED 0x00000001
#define OPTION_FLAG_IS_SET 0x00000002
#define OPTION_FLAG_GUC 0x00000004
#define OPTION_FLAG_HELP_DISPLAY 0x00000008

typedef struct ServiceOption
{
	char		name[OPTION_NAME_LEN];
	char		description[OPTION_VALUE_LEN];
	uint32_t	flags;
	char	   *value_ptr;
	size_t		current_len;
	size_t		max_len;
	struct ServiceOption *next;
	struct ServiceOption *prev;
}			ServiceOption;

void		define_new_option(ServiceOption * *option_list, const char *name,
							  char *description, const uint32_t flags, char *storage_ptr,
							  const size_t max_storage_size);
int			set_option_value(ServiceOption * list, const char *name,
							 const char *value, bool concat);
char	   *get_option_value(ServiceOption * list, const char *name);
void		print_service_options(ServiceOption * list, bool print_value,
								  char *text, size_t max_len);
int			get_option_value_length(ServiceOption * list, const char *name);
int			get_option_value_max_length(ServiceOption * list, const char *name);
ServiceOption *get_option(ServiceOption * list, const char *name);

#endif							/* _SERVICE_OPTION_H */
