#ifndef _REST_TRANSFER_H_
#define _REST_TRANSFER_H_

#include "core/ai_service.h"

typedef void (*make_post_header)(char *buffer, const size_t maxlen,
								 const char *data, const size_t len);
void rest_transfer(AIService *ai_service);
void init_rest_transfer(AIService *ai_service);
void cleanup_rest_transfer(AIService *ai_service);

#endif /* _REST_TRANSFER_H_ */
