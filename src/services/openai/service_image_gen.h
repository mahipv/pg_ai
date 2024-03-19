#ifndef _SERVICE_IMAGE_GEN_H_
#define _SERVICE_IMAGE_GEN_H_

#include "ai_service.h"

/* calls made from Pg <-> PgAi */
void		image_gen_help(char *help_text, const size_t max_len);
void		image_gen_init_service_options(void *service);
int			image_gen_set_and_validate_options(void *service,
											   void *function_options);
int			image_gen_init_service_data(void *options, void *ai_service,
										void *key);
int			image_gen_cleanup_service_data(void *ai_service);

/* call backs from REST <-> PgAi */
void		image_gen_rest_transfer(void *ai_service);
void		image_gen_set_service_buffers(RestRequest * rest_request,
										  RestResponse * rest_response,
										  ServiceData * service_data);
void		image_gen_post_header_maker(char *buffer, const size_t maxlen,
										const char *data, const size_t len);
int			image_gen_add_service_headers(CURL * curl,
										  struct curl_slist **headers,
										  void *service);

#endif							/* _SERVICE_IMAGE_GEN_H_ */
