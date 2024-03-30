#include "ai_service.h"

#include <funcapi.h>

#include "guc/pg_ai_guc.h"
#include "ai_service_init_oai.h"

/*
 * Clear the AIService data
 */
void reset_service(AIService *ai_service)
{
	if (ai_service->memory_context)
		MemoryContextReset(ai_service->memory_context);
}

/*
 * Initialize the service and model based on the GUCs.
 */
static int init_service_model_from_guc(AIService *ai_service,
									   size_t *service_flags,
									   size_t *model_flags)
{
	char *service_name;
	char *model_name;

	service_name = get_pg_ai_guc_string_variable(PG_AI_GUC_SERVICE);
	if (!service_name)
		*service_flags = SERVICE_OPENAI;

	/* if model name is not set use the defaults */
	model_name = get_pg_ai_guc_string_variable(PG_AI_GUC_MODEL);
	if (!model_name)
	{
		*service_flags = SERVICE_OPENAI;
		if (ai_service->function_flags &
			(FUNCTION_GET_INSIGHT | FUNCTION_GET_INSIGHT_AGGREGATE))
			*model_flags = MODEL_OPENAI_GPT;
		else if (ai_service->function_flags &
				 (FUNCTION_CREATE_VECTOR_STORE | FUNCTION_QUERY_VECTOR_STORE))
			*model_flags = MODEL_OPENAI_EMBEDDINGS;
		else if (ai_service->function_flags &
				 (FUNCTION_MODERATION | FUNCTION_MODERATION_AGGREGATE))
			*model_flags = MODEL_OPENAI_MODERATION;
		else if (ai_service->function_flags &
				 (FUNCTION_GENERATE_IMAGE | FUNCTION_GENERATE_IMAGE_AGGREGATE))
			*model_flags = MODEL_OPENAI_IMAGE_GEN;
		return RETURN_ZERO;
	}

	return RETURN_ERROR;
}

/*
 * Set the common options for the service and model.
 */
static void set_common_options(AIService *ai_service, const char *service_name,
							   const char *model_name, const char *model_url)
{
	char *api_key;
	int *debug_level;

	set_option_value(AI_SERVICE_OPTIONS, OPTION_SERVICE_NAME, service_name,
					 false /* concat */);
	set_option_value(AI_SERVICE_OPTIONS, OPTION_MODEL_NAME, model_name,
					 false /* concat */);
	set_option_value(AI_SERVICE_OPTIONS, OPTION_ENDPOINT_URL, model_url,
					 false /* concat */);

	/* set the API key if it is available in a GUC */
	api_key = get_pg_ai_guc_string_variable(PG_AI_GUC_API_KEY);
	if (api_key)
		set_option_value(AI_SERVICE_OPTIONS, OPTION_SERVICE_API_KEY, api_key,
						 false /* concat */);

	/* set the debug level if it is available in a GUC */
	if ((debug_level = get_pg_ai_guc_int_variable(PG_AI_GUC_DEBUG_LEVEL)))
		ai_service->debug_level = *debug_level;
}

/*
 * Given the specific service and model, create the service.
 */
int initialize_service(size_t service_flags, size_t model_flags,
					   AIService *ai_service)
{
	int return_value = RETURN_ERROR;
	char service_name[PG_AI_NAME_LENGTH];
	char service_description[PG_AI_DESC_LENGTH];
	char model_name[PG_AI_NAME_LENGTH];
	char model_description[PG_AI_DESC_LENGTH];
	char model_url[SERVICE_DATA_SIZE];

	/* initialize the service based on the service and model flags */
	if (service_flags & SERVICE_OPENAI)
	{
		strcpy(service_name, SERVICE_OPENAI_NAME);
		strcpy(service_description, SERVICE_OPENAI_DESCRIPTION);
		return_value = create_service_oai(model_flags, ai_service, model_name,
										  model_description, model_url);
	}

	/* service supported, fill in the pointers for common functions */
	if (return_value == RETURN_ZERO)
	{
		/* setup common vtable */
		ai_service->get_service_name = get_service_name;
		ai_service->get_service_description = get_service_description;
		ai_service->get_model_name = get_model_name;
		ai_service->get_model_description = get_model_description;
		ai_service->define_common_options = define_common_options;

		/* initialize the service data structure */
		create_service_data(ai_service, service_description, model_description);

		/* initialize service data and define options for the service */
		(ai_service->init_service_options)(ai_service);

		/* set the common options */
		set_common_options(ai_service, service_name, model_name, model_url);
	}
	return return_value;
}

/*
 * Initialize the AIService based on the service parameter.
 */
int create_service(AIService *ai_service)
{
	size_t service_flags = 0;
	size_t model_flags = 0;

	/* initialize the service and model from the GUCs */
	if (init_service_model_from_guc(ai_service, &service_flags, &model_flags))
		return RETURN_ERROR;

	/* create the service based on the flags */
	if (initialize_service(service_flags, model_flags, ai_service))
		return RETURN_ERROR;

	return RETURN_ZERO;
}

/*
 * Define the common options for the services and models.
 */
void define_common_options(void *service)
{
	AIService *ai_service = (AIService *)service;
	ServiceOption **option_list = &(AI_SERVICE_OPTIONS);

	/* common options for the services */
	define_new_option(option_list, OPTION_SERVICE_NAME,
					  OPTION_SERVICE_NAME_DESC,
					  OPTION_FLAG_REQUIRED | OPTION_FLAG_GUC,
					  NULL /* storage ptr */, 0 /* max size */);

	define_new_option(option_list, OPTION_MODEL_NAME, OPTION_MODEL_NAME_DESC,
					  OPTION_FLAG_REQUIRED | OPTION_FLAG_GUC,
					  NULL /* storage ptr */, 0 /* max size */);

	define_new_option(option_list, OPTION_ENDPOINT_URL,
					  OPTION_ENDPOINT_URL_DESC,
					  OPTION_FLAG_REQUIRED | OPTION_FLAG_GUC,
					  NULL /* storage ptr */, 0 /* max size */);

	define_new_option(option_list, OPTION_SERVICE_API_KEY,
					  OPTION_SERVICE_API_KEY_DESC,
					  OPTION_FLAG_REQUIRED | OPTION_FLAG_GUC,
					  NULL /* storage ptr */, 0 /* max size */);
}

/*
 * Generic callback to return the AI service's provider.
 */
const char *get_service_name(const AIService *ai_service)
{
	return (get_option_value(AI_SERVICE_OPTIONS, OPTION_SERVICE_NAME));
}

/*
 * Generic callback to return the AI service description.
 */
const char *get_service_description(const AIService *ai_service)
{
	return (AI_SERVICE_DATA->service_description);
}

/*
 * Generic callback to return the AI model name.
 */
const char *get_model_name(const AIService *ai_service)
{
	return (get_option_value(AI_SERVICE_OPTIONS, OPTION_MODEL_NAME));
}

/*
 * Generic callback to return the AI model description.
 */
const char *get_model_description(const AIService *ai_service)
{
	return (AI_SERVICE_DATA->model_description);
}

/*
 * Create the service data structure for the AI service.
 */
ServiceData *create_service_data(AIService *ai_service,
								 const char *service_description,
								 const char *model_description)
{
	AI_SERVICE_DATA =
		MemoryContextAllocZero(ai_service->memory_context, sizeof(ServiceData));

	/* set the service and model names */
	strcpy(AI_SERVICE_DATA->service_description, service_description);
	strcpy(AI_SERVICE_DATA->model_description, model_description);

	/* get the max request and response sizes from the respective services */
	ai_service->get_max_request_response_sizes(
		&(AI_SERVICE_DATA->max_request_size),
		&(AI_SERVICE_DATA->max_response_size));

	/* allocate the request and response buffers accordingly */
	AI_SERVICE_DATA->request_data = MemoryContextAllocZero(
		ai_service->memory_context, AI_SERVICE_DATA->max_request_size);
	AI_SERVICE_DATA->request_data[0] = '\0';

	AI_SERVICE_DATA->response_data = MemoryContextAllocZero(
		ai_service->memory_context, AI_SERVICE_DATA->max_response_size);
	AI_SERVICE_DATA->response_data[0] = '\0';

	return AI_SERVICE_DATA;
}