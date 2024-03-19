#ifndef _SERVICE_GPT_H_
#define _SERVICE_GPT_H_

#include "ai_service.h"

/* calls made from Pg <-> PgAi */
int			gpt_init_service_data(void *options, void *ai_service, void *key);
int			gpt_cleanup_service_data(void *ai_service);
void		gpt_help(char *help_text, const size_t max_len);
void		gpt_init_service_options(void *service);
int			gpt_set_and_validate_options(void *service, void *function_options);

/* call backs from REST <-> PgAi */
void		gpt_rest_transfer(void *ai_service);
void		gpt_post_header_maker(char *buffer, const size_t maxlen,
								  const char *data, const size_t len);
int			gpt_add_service_headers(CURL * curl, struct curl_slist **headers,
									void *service);
void		gpt_set_service_buffers(RestRequest * rest_request,
									RestResponse * rest_response,
									ServiceData * service_data);

#endif							/* _SERVICE_GPT_H_ */
