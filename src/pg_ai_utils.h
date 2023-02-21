#ifndef _PG_AI_UTILS_H_
#define _PG_AI_UTILS_H_

#include <ctype.h>
#include <stdio.h>

int 	escape_encode(const char* src, const size_t src_len,
					  char *dst, const size_t max_dst_len);
int 	get_word_count(const char *text, const size_t max_allowed, size_t *actual_count);
int		get_option_from_file(const char *file_path, const char *provider_name,
							 const char *service_name, const int is_provider_option,
							 const char *option_name, char *option_value,
							 const size_t max_buffer_len);
#endif /* _PG_AI_UTILS_H_ */
