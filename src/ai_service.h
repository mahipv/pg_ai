#ifndef _AI_SERVICE_H_
#define _AI_SERVICE_H_

#include <stdio.h>
#include "ai_config.h"

/*
 * struct for the curl REST request info
 */
typedef struct RestRequestData
{
	void		*headers;
	void		*data;
	size_t  	data_size;
	size_t  	max_size;
	void		*prompt;
}RestRequest;

/*
 * struct for the curl REST response info
 */
typedef struct RestResponseData
{
	long     	response_code;
	void		*headers;
	void		*data;
	size_t  	data_size;
	size_t  	max_size;
}RestResponse;

/*
 * struct to handle AI service specific information
 */
typedef struct ServiceData
{
	char			provider[PG_AI_NAME_LENGTH];
	char			name[PG_AI_NAME_LENGTH];
	char			description[PG_AI_NAME_LENGTH];
	char			url[SERVICE_DATA_SIZE];
	char			key[SERVICE_DATA_SIZE];
	char 			request[SERVICE_MAX_RESPONSE_SIZE];
	size_t  		max_request_size;
	char 			prompt[SERVICE_DATA_SIZE];
	size_t  		max_prompt_size;
	char    		response[SERVICE_MAX_RESPONSE_SIZE];
	size_t  		max_response_size;
	ServiceOption 	*options;
}ServiceData;

/* PG <-> AIService Interactions  - helpers */
typedef const char* 	(*GetServiceProvider)();
typedef const char* 	(*GetServiceName)();
typedef const char* 	(*GetServiceDescription)();
typedef void 	(*GetServiceHelp)(char *help_text, const size_t max_len);
typedef void    (*InitServiceOptions)(void *ai_service);
typedef int 	(*InitServiceData)(void *options, void *ai_service, void *key);
typedef void 	(*RestTransfer)(void *ai_service);
typedef int 	(*CleanupServiceData)(void *ai_service);
typedef int     (*SetAndValidateOptions)(void *service, void *function_params);
/* AIService <-> Curl Interactions */
typedef void 	(*SetServiceBuffers)(RestRequest *rest_request,
									 RestResponse *rest_response,
									 ServiceData *service_data);
typedef int 	(*AddServiceHeaders)(CURL *curl, struct curl_slist **headers, void *service);
typedef void 	(*PostHeaderMaker)(char *buffer, const size_t maxlen,
								 const char *data, const size_t len);

/*
 * The master structure that has the "vtable" pointing to the functions of the
 * AI service being used.
 */
typedef struct AIService
{
	/* PG <-> AIService Interactions - helpers */
	GetServiceProvider			get_provider_name;
	GetServiceName				get_service_name;
	GetServiceDescription		get_service_description;
	GetServiceHelp				get_service_help;
	InitServiceOptions			init_service_options;
	SetAndValidateOptions       set_and_validate_options;
	InitServiceData				init_service_data;
	RestTransfer	 			rest_transfer;
	CleanupServiceData			cleanup_service_data;

	/* AIService <-> Curl Interactions */
	SetServiceBuffers           set_service_buffers;
	/* TODO to be implemented with the request/response header/data handling */
	PostHeaderMaker				post_header_maker;
	AddServiceHeaders           add_service_headers;
	ServiceData					*service_data;
	RestRequest 				*rest_request;
	RestResponse 				*rest_response;
	/* TODO Convert this to flags */
	int 						function_flags;
} AIService;

void 			initialize_self(AIService *ai_service);
int 			initialize_service(const char *name, AIService *ai_service);
const char* 	get_provider(const AIService *ai_service);
const char* 	get_service_name(const AIService *ai_service);
const char* 	get_service_description(const AIService *ai_service);
void 			get_service_options(const AIService *ai_service, char *text, const size_t max_len);

#endif /* _AI_SERVICE_H_ */
