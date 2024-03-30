#ifndef _SERVICE_GPT_H_
#define _SERVICE_GPT_H_

#include "core/ai_service.h"

/* calls made from Pg <-> PgAi */
void gpt_initialize_service(void *service);
void gpt_get_max_request_response_sizes(size_t *max_request_size,
										size_t *max_response_size);
int gpt_set_and_validate_options(void *service, void *function_options);
int gpt_set_service_data(void *ai_service, void *data);
int gpt_prepare_for_transfer(void *service);
void gpt_help(char *help_text, const size_t max_len);
int gpt_cleanup_service_data(void *ai_service);

/* call backs from REST <-> PgAi */
void gpt_rest_transfer(void *ai_service);
void gpt_set_service_buffers(RestRequest *rest_request,
							 RestResponse *rest_response,
							 ServiceData *service_data);
int gpt_add_rest_headers(CURL *curl, struct curl_slist **headers,
						 void *service);
void gpt_add_rest_data(char *buffer, const size_t maxlen, const char *data,
					   const size_t len);

#endif /* _SERVICE_GPT_H_ */
