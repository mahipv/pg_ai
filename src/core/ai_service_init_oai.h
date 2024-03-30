#ifndef AI_SERVICE_INIT_OAI_H_
#define AI_SERVICE_INIT_OAI_H_

#include "ai_service.h"

int create_service_oai(size_t model_flags, AIService *ai_service,
					   char *model_name, char *model_description,
					   char *model_url);

#endif /* AI_SERVICE_INIT_OAI_H_ */