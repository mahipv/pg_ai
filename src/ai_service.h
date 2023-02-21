#ifndef _AI_SERVICE_H_
#define _AI_SERVICE_H_

#include "ai_config.h"
#include <stdio.h>


/* Functions should be implemented by all ai services */

/* function in every call back that initializes other callbacks */
typedef int (*InitializeCallBacks)(void *ai_service_callbacks);

/* PG <-> AIService Interactions */
typedef void 	(*GetServiceName)(char *name, const size_t max_len);
typedef void 	(*GetServiceDescription)(char *description, const size_t max_len);
typedef void 	(*GetServiceHelp)(char *help_text, const size_t max_len);

typedef void 	(*RestTransfer)(void *ai_service);
typedef int 	(*InitServiceData)(void *params, void *ai_service, void *key);
typedef int 	(*CleanupServiceData)(void *ai_service);

/* AIService <-> Curl Interactions */
typedef int 	(*MakePostHeaderString)(char *buffer, const size_t maxlen,
										const char *data, const size_t len);
typedef void	(*SetServiceBuffers)(RestRequest *rest_request,
									 RestResponse *rest_response,
									 ServiceData *service_data);
typedef void    (*PostHeaderMaker)(char *buffer, const size_t maxlen,
								 const char *data, const size_t len);
typedef struct AIService
{
	char						name[PG_AI_LINE_LENGTH];
	ServiceData					*service_data;
	RestRequest 				*rest_request;
	RestResponse 				*rest_response;
	/* TODO Convert this to flags */
	int		 				is_aggregate;

	/* PG <-> AIService Interactions */
	GetServiceName				get_service_name;
	GetServiceDescription		get_service_description;
	GetServiceHelp				get_service_help;
	InitServiceData				init_service_data;
	CleanupServiceData			cleanup_service_data;
	RestTransfer	 			rest_transfer;

	/* AIService <-> Curl Interactions */
	SetServiceBuffers           set_service_buffers;
	/* TODO to be implemented with the request/response header/data handling */
	PostHeaderMaker				post_header_maker;
} AIService;

void initialize_self(AIService *ai_service);
int initialize_service(const char *name, AIService *ai_service);

#endif /* _AI_SERVICE_H_ */
