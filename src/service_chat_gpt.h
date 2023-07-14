#ifndef _SERVICE_CHAT_GPT_H_
#define _SERVICE_CHAT_GPT_H_

#include "postgres.h"
#include "common/jsonapi.h"
#include "ai_service.h"

int 	chat_gpt_init_service_data(void *options, void *ai_service, void *key);
int 	chat_gpt_cleanup_service_data(void *ai_service);
void 	chat_gpt_help(char *help_text, const size_t max_len);
void 	chat_gpt_rest_transfer(void *ai_service);
void 	chat_gpt_set_service_buffers(RestRequest *rest_request, RestResponse *rest_response,
									 ServiceData *service_data);
void 	chat_gpt_post_header_maker(char *buffer, const size_t maxlen, const char *data,
								   const size_t len);
int 	chat_gpt_add_service_headers(CURL *curl, struct curl_slist **headers, void *service);
void 	chat_gpt_init_service_options(void *service);
int     chat_gpt_set_and_validate_options(void *service, void *function_options);
#endif /* _SERVICE_CHAT_GPT_H_ */
