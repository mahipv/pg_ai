#include "pg_ai_utils.h"
#include <stdlib.h>
#include <string.h>
#include "ai_config.h"
#include "postgres.h"
#include "utils/builtins.h"

/*
 * Function to escape the special characters that are to be sent as data
 * to a REST service. Make sure the dst array passed to this method can
 * accomodate the source string along with the escaped characters.
 *
 * @param[in]	src				the source string which has to be escaped
 * @param[in]	src_len			length of the src string
 * @param[out]	dst				the dst for the escaped string
 * @param[in]	max_dst_lenm	max length of dst array
 * @return		zero on success, non-zero otherwise
 *
 */
int
escape_encode(const char* src, const size_t src_len, char *dst,
			  const size_t max_dst_len)
{
	const char	*p = src;
	char		*q = dst;
	size_t		src_index = 0;
	size_t		dst_index = 0;
	int			byte_count;

    while (src_index++ < src_len)
	{
        if (isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~')
		{
            *q++ = *p;
			dst_index++;
        }
		else
		{
			byte_count = sprintf(q, "%%%02X", (unsigned char) *p);

			/* if dst buffer cannot handle the new length return error */
			if (dst_index + byte_count > max_dst_len)
				return 1;

            q += byte_count;
			dst_index += byte_count;
        }
        p++;
    }
    *q = '\0';
	return 0;
}

/*
 * Given a string, get the number of words. If the word count is more
 * than a given max threshold, the counting is stopped.
 *
 * @param[in]	text			text in which the words have to be counted
 * @param[in]	max_allowed		the max count that this method counts
 * @param[out]	actual_count	the actual word count in the text, valid only
 *								if the words are < max_allowed
 * @return		zero on success, non-zero otherwise
 *
 */
int
get_word_count(const char *text, const size_t max_allowed, size_t *actual_count)
{
	size_t 	count = 0;
	int 	i = 0;

	while (text[i] != '\0' && count < max_allowed)
	{
		if (text[i] == ' ' && text[i+1] != ' ')
			count++;
		i++;
	}

	if (count < max_allowed)
	{
		if (actual_count)
			*actual_count = count;
		return RETURN_ZERO;
	}

	return RETURN_ERROR;
}

/*
 * Given a AI service provider and the service name, reads the value of a
 * option from a json options file.
 *
 * @param[in]	file_path 			path to the json options file
 * @param[in]	provider_name		AI sservice provider
 * @param[in]	service_name		AI service
 * @param[in]	is_provider_option	true if the option is for all services
 *									provided by the same provider
 * @param[in]	option_name			name of the option whose value to be read
 * @param[out]	option_value		value of the option
 * @param[in]	max_buffer_lenq		max buffer length the value can take
 * @return		zero on success, non-zero otherwise
 *
 */
int
get_option_from_file(const char *file_path, const char *provider_name,
					 const char *service_name, const int is_provider_option,
					 const char *option_name, char *option_value,
					 const size_t max_buffer_len)
{
	Datum		choices_json = 0;
	Datum		provider_data_json = 0;
	Datum       provider_name_json = 0;
	Datum		service_data_json = 0;
	Datum       service_name_json = 0;
	Datum		option_value_json = 0;
	char		buffer[OPTIONS_FILE_SIZE];
	FILE 		*fp;
	int			ret_val=1;
	size_t		file_size;
	int			i = 0;

	PG_TRY();
	{
		fp = fopen(file_path, "r");
		if (!fp)
			return ret_val;

		/* get the file size */
		fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		rewind(fp);
		if (file_size > OPTIONS_FILE_SIZE)
			return ret_val;

		/* read the file into the buffer */
		file_size = fread(buffer, 1, file_size, fp);
		buffer[file_size] = '\0';

		/* */
		choices_json = DirectFunctionCall2(json_object_field_text,
										   CStringGetTextDatum(buffer),
										   PointerGetDatum(cstring_to_text(OPTION_PROVIDERS)));
		/* look into each provder of the json input */
		while (1)
		{
			provider_data_json = DirectFunctionCall2(json_array_element_text,
													 choices_json,
													 PointerGetDatum(i++));
			if (!provider_data_json)
				break;

			/* get the name of this provider */
			provider_name_json = DirectFunctionCall2(
				json_object_field_text,
				provider_data_json,
				PointerGetDatum(cstring_to_text(OPTION_PROVIDER_NAME)));

			/* found the provider */
			if (!strcmp(text_to_cstring(DatumGetTextPP(provider_name_json)), provider_name))
				break;
		}

		/* if a provider option get the value */
		if (provider_name_json && is_provider_option)
		{
			option_value_json = DirectFunctionCall2(json_object_field_text,
												   provider_data_json,
												   PointerGetDatum(
													   cstring_to_text(option_name)));
			if (option_value_json)
			{
				strncpy(option_value, text_to_cstring(DatumGetTextPP(option_value_json)),
						max_buffer_len);
				ret_val = 0;
			}
		}
		else
		{
			/* search for the option in the services */
			choices_json = DirectFunctionCall2(json_object_field_text,
											   provider_data_json,
											   PointerGetDatum(
												   cstring_to_text(OPTION_SERVICES)));
			i = 0;
			while (1)
			{
				service_data_json = DirectFunctionCall2(json_array_element_text,
														choices_json,
														PointerGetDatum(i++));
				if (!service_data_json)
					break;

				/* get the name of this service */
				service_name_json = DirectFunctionCall2(
					json_object_field_text,
					service_data_json,
					PointerGetDatum(cstring_to_text(OPTION_SERVICE_NAME)));

				/* found the service, now get the option value */
				if (!strcmp(text_to_cstring(DatumGetTextPP(service_name_json)), service_name))
				{
					option_value_json = DirectFunctionCall2(json_object_field_text,
														   service_data_json,
														   PointerGetDatum(
															   cstring_to_text(option_name)));
					if (option_value_json)
					{
						ret_val = 0;
						strncpy(option_value, text_to_cstring(DatumGetTextPP(option_value_json)),
								max_buffer_len);
						break;
					}
				}
			}
		}
	}
	PG_CATCH();
	{
		ret_val = 1;
	}
	PG_END_TRY();
	return ret_val;
}
