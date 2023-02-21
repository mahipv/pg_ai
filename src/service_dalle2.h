#ifndef _SERVICE_DALLE_2_H_
#define _SERVICE_DALLE_2_H_

#include "postgres.h"
#include "common/jsonapi.h"

#include "rest_transfer.h"
#include "postgres.h"
#include "common/jsonapi.h"

#include "ai_service.h"

void dalle2_name(char *name, const size_t max_len);
void dalle2_description(char *description, const size_t max_len);
void dalle2_help(char *help_text, const size_t max_len);

int dalle2_init_service_data(void * params, void *ai_service, void *key);
int dalle2_cleanup_service_data(void *ai_service);

void dalle2_rest_transfer(void *ai_service);
void dalle2_set_service_buffers(RestRequest *rest_request,
								  RestResponse *rest_response,
								  ServiceData *service_data);

void dalle2_post_header_maker(char *buffer, const size_t maxlen,
							  const char *data, const size_t len);

#endif /* _SERVICE_DALLE_2_H_ */
