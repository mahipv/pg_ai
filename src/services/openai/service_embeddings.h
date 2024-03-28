#ifndef _SERVICE_EMBEDDINGS_H_
#define _SERVICE_EMBEDDINGS_H_

#include "core/ai_service.h"

#define EMBEDDINGS_SIMILARITY_COSINE "cosine"
#define EMBEDDINGS_SIMILARITY_EUCLIDEAN "euclidean"
#define EMBEDDINGS_SIMILARITY_INNER_PRODUCT "inner_product"

/* calls made from Pg <-> PgAi */
void embeddings_initialize_service(void *service);
int embeddings_set_and_validate_options(void *service, void *function_options);
int embeddings_set_service_data(void *ai_service, void *data);
int embeddings_prepare_for_transfer(void *service);
void embeddings_help(char *help_text, const size_t max_len);
int embeddings_cleanup_service_data(void *ai_service);

/* call backs from REST <-> PgAi */
void embeddings_rest_transfer(void *ai_service);
void embeddings_set_service_buffers(RestRequest *rest_request,
									RestResponse *rest_response,
									ServiceData *service_data);
int embeddings_add_rest_headers(CURL *curl, struct curl_slist **headers,
								void *service);
void embeddings_add_rest_data(char *buffer, const size_t maxlen,
							  const char *data, const size_t len);

/* TODO */
int execute_query_spi(const char *query, bool read_only);
int embeddings_handle_response_headers(void *service, void *user_data);
int embeddings_handle_response_data(void *service, void *user_data);

void set_similarity_algorithm(ServiceOption *options);
#endif /* _SERVICE_EMBEDDINGS_H_ */
