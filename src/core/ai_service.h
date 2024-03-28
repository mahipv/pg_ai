#ifndef _AI_SERVICE_H_
#define _AI_SERVICE_H_

#include <stdio.h>

#include "postgres.h"

#include "ai_config.h"
#include "ai_error.h"

/*
 * struct for the curl REST request info
 */
typedef struct RestRequestData
{
	void *headers;
	void *data;
	size_t data_size;
	size_t max_size;
	void *prompt;
} RestRequest;

/*
 * struct for the curl REST response info
 */
typedef struct RestResponseData
{
	long response_code;
	void *headers;
	void *data;
	size_t data_size;
	size_t max_size;
} RestResponse;

/*
 * struct to handle AI service specific information
 */
typedef struct ServiceData
{
	char service_description[PG_AI_DESC_LENGTH];
	char model_description[PG_AI_DESC_LENGTH];
	char request[SERVICE_MAX_RESPONSE_SIZE];
	size_t max_request_size;
	char prompt[SERVICE_DATA_SIZE];
	size_t max_prompt_size;
	char response[SERVICE_MAX_RESPONSE_SIZE];
	size_t max_response_size;
	ServiceOption *options;
	void **user_data_ptr;
} ServiceData;

/* PG <-> AIService Interactions  - helpers */
typedef const char *(*GetServiceName)();
typedef const char *(*GetServiceDescription)();
typedef const char *(*GetModelName)();
typedef const char *(*GetModelDescription)();
typedef void (*GetServiceHelp)(char *help_text, const size_t max_len);

/* PG <-> AIService Interactions */
typedef void (*InitServiceOptions)(void *ai_service);
typedef int (*SetAndValidateOptions)(void *service, void *function_params);
typedef int (*PrepareForTransfer)(void *ai_service);
typedef int (*SetServiceData)(void *ai_service, void *data);
typedef void (*RestTransfer)(void *ai_service);
typedef int (*CleanupServiceData)(void *ai_service);

/* Curl <-> AIService Interactions */
typedef void (*SetServiceBuffers)(RestRequest *rest_request,
								  RestResponse *rest_response,
								  ServiceData *service_data);
typedef int (*AddRestHeaders)(CURL *curl, struct curl_slist **headers,
							  void *service);
typedef void (*AddRestData)(char *buffer, const size_t maxlen, const char *data,
							const size_t len);

/* PgAI internal calls */
typedef void (*DefineCommonOptions)(void *service);

/*
 * The master structure that has the "vtable" pointing to the functions of the
 * AI service being used.
 */
typedef struct AIService
{
	/* flags to represent a AI service */
	int service_flags;

	/* flags to represent a model in of the service */
	int model_flags;

	/* flags to represent the SQL function being called */
	int function_flags;

	/* flags to represent the debug level */
	int debug_level;

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

	/* set the service data specific to the model */
	SetServiceData set_service_data;

	/* Prepare the data for tarnsfer */
	PrepareForTransfer prepare_for_transfer;

	/* the function to make the final REST transfer using curl */
	RestTransfer rest_transfer;

	/* cleanup the service data */
	CleanupServiceData cleanup_service_data;

	/* Curl <-> PgAi interaction functions to be implemented by every model */

	/* set the buffers for the REST request and response */
	SetServiceBuffers set_service_buffers;

	/* call back to add the service/model headers to the curl POST request */
	AddRestHeaders add_rest_headers;

	/* call back to add data to the curl POST request */
	AddRestData add_rest_data;

	/* PgAI internal calls */
	DefineCommonOptions define_common_options;

} AIService;

void reset_service(AIService *ai_service);
int initialize_service(const int service_flags, const int model_flags,
					   AIService *ai_service);
const char *get_service_name(const AIService *ai_service);
const char *get_service_description(const AIService *ai_service);
const char *get_model_name(const AIService *ai_service);
const char *get_model_description(const AIService *ai_service);
void define_common_options(void *ai_service);

/* Implementations of the SQL functions to call these macros in order */
#define INITIALIZE_SERVICE(service, model, ai_service)                         \
	do                                                                         \
	{                                                                          \
		if (initialize_service(service, model, ai_service))                    \
			PG_RETURN_TEXT_P(GET_ERR_TEXT(UNSUPPORTED_SERVICE));               \
	} while (0)

#define SET_AND_VALIDATE_OPTIONS(ai_service, fcinfo)                           \
	do                                                                         \
	{                                                                          \
		if (((SetAndValidateOptions)(ai_service->set_and_validate_options))(   \
				ai_service, fcinfo))                                           \
			PG_RETURN_TEXT_P(GET_ERR_TEXT(INVALID_OPTIONS));                   \
	} while (0)

#define SET_SERVICE_DATA(ai_service, data)                                     \
	do                                                                         \
	{                                                                          \
		if (((SetServiceData)(ai_service->set_service_data))(ai_service,       \
															 data))            \
			PG_RETURN_TEXT_P(GET_ERR_TEXT(INT_DATA_ERR));                      \
	} while (0)

#define PREPARE_FOR_TRANSFER(ai_service)                                       \
	do                                                                         \
	{                                                                          \
		if (((PrepareForTransfer)(ai_service->prepare_for_transfer))(          \
				ai_service))                                                   \
			PG_RETURN_TEXT_P(GET_ERR_TEXT(INT_PREP_TNSFR));                    \
	} while (0)

#define REST_TRANSFER(ai_service)                                              \
	((RestTransfer)(ai_service->rest_transfer))(ai_service)

#endif /* _AI_SERVICE_H_ */
