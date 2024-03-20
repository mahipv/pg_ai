#ifndef _AI_SERVICE_H_
#define _AI_SERVICE_H_

#include <stdio.h>

#include "postgres.h"

#include "ai_config.h"

/*
 * struct for the curl REST request info
 */
typedef struct RestRequestData
{
	void	   *headers;
	void	   *data;
	size_t		data_size;
	size_t		max_size;
	void	   *prompt;
}			RestRequest;

/*
 * struct for the curl REST response info
 */
typedef struct RestResponseData
{
	long		response_code;
	void	   *headers;
	void	   *data;
	size_t		data_size;
	size_t		max_size;
}			RestResponse;

/*
 * struct to handle AI service specific information
 */
typedef struct ServiceData
{
	char		service_description[PG_AI_DESC_LENGTH];
	char		model_description[PG_AI_DESC_LENGTH];
	char		request[SERVICE_MAX_RESPONSE_SIZE];
	size_t		max_request_size;
	char		prompt[SERVICE_DATA_SIZE];
	size_t		max_prompt_size;
	char		response[SERVICE_MAX_RESPONSE_SIZE];
	size_t		max_response_size;
	ServiceOption *options;
	void	  **user_data_ptr;
}			ServiceData;

/* PG <-> AIService Interactions  - helpers */
typedef const char *(*GetServiceName) ();
typedef const char *(*GetServiceDescription) ();
typedef const char *(*GetModelName) ();
typedef const char *(*GetModelDescription) ();
typedef void (*GetServiceHelp) (char *help_text, const size_t max_len);
typedef void (*InitServiceOptions) (void *ai_service);
typedef int (*InitServiceData) (void *options, void *ai_service, void *key);
typedef void (*RestTransfer) (void *ai_service);
typedef int (*CleanupServiceData) (void *ai_service);
typedef int (*SetAndValidateOptions) (void *service, void *function_params);

/* Curl <-> AIService Interactions */
typedef void (*SetServiceBuffers) (RestRequest * rest_request,
								   RestResponse * rest_response,
								   ServiceData * service_data);
typedef int (*AddServiceHeaders) (CURL * curl, struct curl_slist **headers,
								  void *service);
typedef int (*AddServiceData) (CURL * curl, struct curl_slist **headers,
							   void *service);
typedef void (*PostHeaderMaker) (char *buffer, const size_t maxlen,
								 const char *data, const size_t len);
typedef int (*HandleResponseHeaders) (void *service, void *user_data);
typedef int (*HandleResponseData) (void *service, void *user_data);


/*
 * The master structure that has the "vtable" pointing to the functions of the
 * AI service being used.
 */
typedef struct AIService
{
	/* flags to represent a AI service */
	int			service_flags;

	/* flags to represent a model in of the service */
	int			model_flags;

	/* flags to represent the SQL function being called */
	int			function_flags;

	/* every service gets to maintain its own private data */
	ServiceData *service_data;

	/* the REST request and response data */
	RestRequest *rest_request;
	RestResponse *rest_response;

	/* memory context for this service will be deleted by end of call */
	MemoryContext memory_context;

	/* PG <-> PgAi Interaction functions to be implemented by every model */

	/* returns the name of the current service */
	GetServiceName get_service_name;

	/* one line description of the service */
	GetServiceDescription get_service_description;

	/* model name */
	GetModelName get_model_name;

	/* one line description of the model in use */
	GetModelDescription get_model_description;

	/* to return the help text of the model */
	GetServiceHelp get_service_help;

	/* define the options to be set based on the function flags */
	InitServiceOptions init_service_options;

	/* validate the options(in case of help() call the options are not set) */
	SetAndValidateOptions set_and_validate_options;

	/* initialize the service data specific to the model */
	InitServiceData init_service_data;

	/* the function to make the final REST transfer using curl */
	RestTransfer rest_transfer;

	/* cleanup the service data */
	CleanupServiceData cleanup_service_data;

	/* Curl <-> PgAi interaction functions to be implemented by every model */

	/* set the buffers for the REST request and response */
	SetServiceBuffers set_service_buffers;

	/* call back to add the service/model headers to the curl POST request */
	AddServiceHeaders add_service_headers;

	/* call back to add data to the curl POST request */
	PostHeaderMaker post_header_maker;

	/* model call back to handle response headers */
	HandleResponseHeaders handle_response_headers;

	/* model call back to handle response data */
	HandleResponseData handle_response_data;
}			AIService;

#define IS_PG_AI_FUNCTION(flag) (ai_service->function_flags & flag)

void		reset_service(AIService * ai_service);
int			initialize_service(const int service_flags, const int model_flags,
							   AIService * ai_service);
const char *get_service_name(const AIService * ai_service);
const char *get_service_description(const AIService * ai_service);
const char *get_model_name(const AIService * ai_service);
const char *get_model_description(const AIService * ai_service);

#endif							/* _AI_SERVICE_H_ */
