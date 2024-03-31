#ifndef _SERVICE_GENC_MOD_H_
#define _SERVICE_GENC_MOD_H_

#include "core/ai_service.h"

/* calls made from Pg <-> PgAi */
void genc_mod_initialize_service(void *service);
void genc_mod_get_max_request_response_sizes(size_t *max_request_size,
											 size_t *max_response_size);
int genc_mod_set_and_validate_options(void *service, void *function_options);
int genc_mod_set_service_data(void *ai_service, void *data);
int genc_mod_prepare_for_transfer(void *service);
void genc_mod_help(char *help_text, const size_t max_len);
int genc_mod_cleanup_service_data(void *ai_service);

/* call backs from REST <-> PgAi */
void genc_mod_rest_transfer(void *ai_service);
void genc_mod_set_service_buffers(RestRequest *rest_request,
								  RestResponse *rest_response,
								  ServiceData *service_data);
int genc_mod_add_rest_headers(CURL *curl, struct curl_slist **headers,
							  void *service);
void genc_mod_add_rest_data(char *buffer, const size_t maxlen, const char *data,
							const size_t len);

#endif /* _SERVICE_GENC_MOD_H_ */