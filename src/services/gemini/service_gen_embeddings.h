#ifndef _SERVICE_GENC_EMBEDDINGS_H_
#define _SERVICE_GENC_EMBEDDINGS_H_

#include "core/ai_service.h"

/* calls made from Pg <-> PgAi */
void gen_embeddings_initialize_service(void *service);
void gen_embeddings_get_max_request_response_sizes(size_t *max_request_size,
												   size_t *max_response_size);
int gen_embeddings_set_and_validate_options(void *service,
											void *function_options);
int gen_embeddings_set_service_data(void *ai_service, void *data);
int gen_embeddings_prepare_for_transfer(void *service);
void gen_embeddings_help(char *help_text, const size_t max_len);
int gen_embeddings_cleanup_service_data(void *ai_service);

/* call backs from REST <-> PgAi */
void gen_embeddings_rest_transfer(void *ai_service);
void gen_embeddings_set_service_buffers(RestRequest *rest_request,
										RestResponse *rest_response,
										ServiceData *service_data);
int gen_embeddings_add_rest_headers(CURL *curl, struct curl_slist **headers,
									void *service);
void gen_embeddings_add_rest_data(char *buffer, const size_t maxlen,
								  const char *data, const size_t len);

/* TODO */
int gen_embeddings_handle_response_headers(void *service, void *user_data);
int gen_embeddings_handle_response_data(void *service, void *user_data);

#endif /* _SERVICE_GENC_EMBEDDINGS_H_ */