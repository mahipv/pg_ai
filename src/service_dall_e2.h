#ifndef _SERVICE_DALL_E2_H_
#define _SERVICE_DALL_E2_H_

#include "postgres.h"
#include "common/jsonapi.h"
#include "rest_transfer.h"
#include "postgres.h"
#include "common/jsonapi.h"
#include "ai_service.h"

void 	dalle_e2_help(char *help_text, const size_t max_len);
void 	dalle_e2_init_service_options(void *service);
int 	dalle_e2_init_service_data(void * options, void *ai_service, void *key);
int 	dalle_e2_cleanup_service_data(void *ai_service);
void 	dalle_e2_rest_transfer(void *ai_service);
void 	dalle_e2_set_service_buffers(RestRequest *rest_request, RestResponse *rest_response,
									 ServiceData *service_data);
void 	dalle_e2_post_header_maker(char *buffer, const size_t maxlen, const char *data,
								   const size_t len);
int 	dalle_e2_add_service_headers(CURL *curl, struct curl_slist **headers, void *service);
int     dalle_e2_set_and_validate_options(void *service, void *function_options);
#endif /* _SERVICE_DALL_E2_H_ */
