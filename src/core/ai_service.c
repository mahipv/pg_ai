#include "ai_service.h"

#include <funcapi.h>

#include "guc/pg_ai_guc.h"
#include "services/openai/service_gpt.h"
#include "services/openai/service_image_gen.h"
#include "services/openai/service_embeddings.h"
#include "services/openai/service_moderation.h"

/*
 * Clear the AIService data
 */
void reset_service(AIService *ai_service)
{
	if (ai_service->memory_context)
		MemoryContextReset(ai_service->memory_context);
}

/*
 * Initialize AIService vtable with the function from the ChatGPT service.
 */
static int initialize_gpt(AIService *ai_service)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = gpt_initialize_service;
	ai_service->set_and_validate_options = gpt_set_and_validate_options;
	ai_service->set_service_data = gpt_set_service_data;
	ai_service->prepare_for_transfer = gpt_prepare_for_transfer;
	ai_service->set_service_buffers = gpt_set_service_buffers;
	ai_service->cleanup_service_data = gpt_cleanup_service_data;
	ai_service->get_service_help = gpt_help;

	/* PgAi <-> REST functions */
	ai_service->rest_transfer = gpt_rest_transfer;
	ai_service->add_rest_headers = gpt_add_rest_headers;
	ai_service->add_rest_data = gpt_add_rest_data;
	return RETURN_ZERO;
}

/*
 * Initialize AIService vtable with the function from the dall-e-2 service.
 */
static int initialize_image_generator(AIService *ai_service)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = image_gen_initialize_service;
	ai_service->set_and_validate_options = image_gen_set_and_validate_options;
	ai_service->set_service_data = image_gen_set_service_data;
	ai_service->prepare_for_transfer = image_gen_prepare_for_transfer;
	ai_service->set_service_buffers = image_gen_set_service_buffers;
	ai_service->cleanup_service_data = image_gen_cleanup_service_data;
	ai_service->get_service_help = image_gen_help;

	/* PgAi <-> REST functions */
	ai_service->rest_transfer = image_gen_rest_transfer;
	ai_service->add_rest_headers = image_gen_add_rest_headers;
	ai_service->add_rest_data = image_gen_add_rest_data;
	return RETURN_ZERO;
}

/*
 * Initialize AIService vtable with functions from the embeddings service.
 */
static int initialize_embeddings(AIService *ai_service)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = embeddings_initialize_service;
	ai_service->set_and_validate_options = embeddings_set_and_validate_options;
	ai_service->set_service_data = embeddings_set_service_data;
	ai_service->prepare_for_transfer = embeddings_prepare_for_transfer;
	ai_service->set_service_buffers = embeddings_set_service_buffers;
	ai_service->cleanup_service_data = embeddings_cleanup_service_data;
	ai_service->get_service_help = embeddings_help;

	/* PgAi <-> REST functions */
	ai_service->rest_transfer = embeddings_rest_transfer;
	ai_service->add_rest_headers = embeddings_add_rest_headers;
	ai_service->add_rest_data = embeddings_add_rest_data;
	return RETURN_ZERO;
}

/*
 * Initialize AIService vtable with the function from the moderation service.
 */
static int initialize_moderation(AIService *ai_service)
{
	/* PG <-> PG_AI functions */
	ai_service->init_service_options = moderation_initialize_service;
	ai_service->set_and_validate_options = moderation_set_and_validate_options;
	ai_service->set_service_data = moderation_set_service_data;
	ai_service->prepare_for_transfer = moderation_prepare_for_transfer;
	ai_service->set_service_buffers = moderation_set_service_buffers;
	ai_service->cleanup_service_data = moderation_cleanup_service_data;
	ai_service->get_service_help = moderation_help;

	/* PG_AI <-> REST functions */
	ai_service->rest_transfer = moderation_rest_transfer;
	ai_service->add_rest_headers = moderation_add_rest_headers;
	ai_service->add_rest_data = moderation_add_rest_data;
	return RETURN_ZERO;
}

/*
 * Initialize the AIService based on the service parameter.
 */
int initialize_service(const int service_flags, const int model_flags,
					   AIService *ai_service)
{
	int return_value = RETURN_ERROR;
	char service_name[PG_AI_NAME_LENGTH];
	char service_description[PG_AI_DESC_LENGTH];
	char model_name[PG_AI_NAME_LENGTH];
	char model_description[PG_AI_DESC_LENGTH];
	char model_url[SERVICE_DATA_SIZE];
	char *api_key;
	int *debug_level;

	/* initialize the service based on the service and model flags */
	if (service_flags & SERVICE_OPENAI)
	{
		strcpy(service_name, SERVICE_OPENAI_NAME);
		strcpy(service_description, SERVICE_OPENAI_DESCRIPTION);
		if (model_flags & MODEL_OPENAI_GPT)
		{
			return_value = initialize_gpt(ai_service);
			strcpy(model_name, MODEL_OPENAI_GPT_NAME);
			strcpy(model_description, MODEL_OPENAI_GPT_DESCRIPTION);
			strcpy(model_url, GPT_API_URL);
		}

		if (model_flags & MODEL_OPENAI_EMBEDDINGS)
		{
			return_value = initialize_embeddings(ai_service);
			strcpy(model_name, MODEL_OPENAI_EMBEDDINGS_NAME);
			strcpy(model_description, MODEL_OPENAI_EMBEDDINGS_DESCRIPTION);
			strcpy(model_url, EMBEDDINGS_API_URL);
		}

		if (model_flags & MODEL_OPENAI_MODERATION)
		{
			return_value = initialize_moderation(ai_service);
			strcpy(model_name, MODEL_OPENAI_MODERATION_NAME);
			strcpy(model_description, MODEL_OPENAI_MODERATION_DESCRIPTION);
			strcpy(model_url, MODERATION_API_URL);
		}

		if (model_flags & MODEL_OPENAI_IMAGE_GEN)
		{
			return_value = initialize_image_generator(ai_service);
			strcpy(model_name, MODEL_OPENAI_IMAGE_GEN_NAME);
			strcpy(model_description, MODEL_OPENAI_IMAGE_GEN_DESCRIPTION);
			strcpy(model_url, IMAGE_GEN_API_URL);
		}
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

		/* initialize service data and define options for the service */
		(ai_service->init_service_options)(ai_service);

		/* set the constant model options */
		set_option_value(ai_service->service_data->options, OPTION_SERVICE_NAME,
						 service_name, false /* concat */);
		set_option_value(ai_service->service_data->options, OPTION_MODEL_NAME,
						 model_name, false /* concat */);
		set_option_value(ai_service->service_data->options, OPTION_ENDPOINT_URL,
						 model_url, false /* concat */);

		/* the service and model descriptions are not options but stored as part
		 * of the service data.
		 */
		strcpy(ai_service->service_data->service_description,
			   service_description);
		strcpy(ai_service->service_data->model_description, model_description);

		/* set the API key if it is available in a GUC */
		api_key = get_pg_ai_guc_string_variable(PG_AI_GUC_API_KEY);
		if (api_key)
			set_option_value(ai_service->service_data->options,
							 OPTION_SERVICE_API_KEY, api_key,
							 false /* concat */);

		/* set the debug level if it is available in a GUC */
		if ((debug_level = get_pg_ai_guc_int_variable(PG_AI_GUC_DEBUG_LEVEL)))
			ai_service->debug_level = *debug_level;
	}
	return return_value;
}

/*
 * Generic callback to return the AI service's provider.
 */
const char *get_service_name(const AIService *ai_service)
{
	return (get_option_value(ai_service->service_data->options,
							 OPTION_SERVICE_NAME));
}

/*
 * Generic callback to return the AI service description.
 */
const char *get_service_description(const AIService *ai_service)
{
	return (ai_service->service_data->service_description);
}

/*
 * Generic callback to return the AI model name.
 */
const char *get_model_name(const AIService *ai_service)
{
	return (
		get_option_value(ai_service->service_data->options, OPTION_MODEL_NAME));
}

/*
 * Generic callback to return the AI model description.
 */
const char *get_model_description(const AIService *ai_service)
{
	return (ai_service->service_data->model_description);
}

/*
 * Define the common options for the services and models.
 */
void define_common_options(void *service)
{
	AIService *ai_service = (AIService *)service;
	ServiceOption **option_list = &(ai_service->service_data->options);

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
