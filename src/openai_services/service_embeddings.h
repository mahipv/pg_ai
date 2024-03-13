#ifndef _SERVICE_EMBEDDINGS_H_
#define _SERVICE_EMBEDDINGS_H_

#include <postgres.h>
#include "common/jsonapi.h"
#include "ai_service.h"

int			embeddings_init_service_data(void *options, void *ai_service, void *key);
int			embeddings_cleanup_service_data(void *ai_service);
void		embeddings_help(char *help_text, const size_t max_len);
void		embeddings_rest_transfer(void *ai_service);
void		embeddings_set_service_buffers(RestRequest * rest_request, RestResponse * rest_response,
										   ServiceData * service_data);
void		embeddings_post_header_maker(char *buffer, const size_t maxlen, const char *data,
										 const size_t len);
int			embeddings_add_service_headers(CURL * curl, struct curl_slist **headers, void *service);
void		embeddings_init_service_options(void *service);

int			execute_query_spi(const char *query, bool read_only);
void		removeNewlines(char *stream);

//move to utils TODO
int			embeddings_set_and_validate_options(void *service, void *function_options);
int			embeddings_handle_response_headers(void *service, void *user_data);
int			embeddings_handle_response_data(void *service, void *user_data);

#endif /* _SERVICE_EMBEDDINGS_H_ */