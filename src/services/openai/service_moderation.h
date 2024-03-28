#ifndef _SERVICE_MODERATION_H_
#define _SERVICE_MODERATION_H_

#include "core/ai_service.h"

/* calls made from Pg <-> PgAi */
void moderation_initialize_service(void *service);
int moderation_set_and_validate_options(void *service, void *function_options);
int moderation_set_service_data(void *ai_service, void *data);
int moderation_prepare_for_transfer(void *service);
void moderation_help(char *help_text, const size_t max_len);
int moderation_cleanup_service_data(void *ai_service);

/* call backs from REST <-> PgAi */
void moderation_rest_transfer(void *ai_service);
void moderation_set_service_buffers(RestRequest *rest_request,
									RestResponse *rest_response,
									ServiceData *service_data);
int moderation_add_rest_headers(CURL *curl, struct curl_slist **headers,
								void *service);
void moderation_add_rest_data(char *buffer, const size_t maxlen,
							  const char *data, const size_t len);
#endif /* _SERVICE_MODERATION_H_ */
