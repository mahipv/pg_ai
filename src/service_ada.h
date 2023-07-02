#ifndef _SERVICE_ADA_H_
#define _SERVICE_ADA_H_

#include "postgres.h"
#include "common/jsonapi.h"
#include "ai_service.h"

int 	ada_init_service_data(void *options, void *ai_service, void *key);
int 	ada_cleanup_service_data(void *ai_service);
void 	ada_help(char *help_text, const size_t max_len);
void 	ada_rest_transfer(void *ai_service);
void 	ada_set_service_buffers(RestRequest *rest_request, RestResponse *rest_response,
									 ServiceData *service_data);
void 	ada_post_header_maker(char *buffer, const size_t maxlen, const char *data,
								   const size_t len);
int 	ada_add_service_headers(CURL *curl, struct curl_slist **headers, void *service);
void 	ada_init_service_options(void *service);

int     execute_query_spi(const char *query);
void removeNewlines(char* stream); // move to utils TODO
#endif /* _SERVICE_ADA_H_ */