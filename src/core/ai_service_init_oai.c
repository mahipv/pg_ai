#include "ai_service_init_oai.h"

#include "services/openai/service_gpt.h"
#include "services/openai/service_image_gen.h"
#include "services/openai/service_embeddings.h"
#include "services/openai/service_moderation.h"

/*
 * Initialize AIService vtable with the function from the ChatGPT service.
 */
static int initialize_gpt(AIService *ai_service, char *model_name,
						  char *model_description, char *model_url)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = gpt_initialize_service;
	ai_service->get_max_request_response_sizes =
		gpt_get_max_request_response_sizes;
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

	/* set the model name and description */
	strcpy(model_name, MODEL_OPENAI_GPT_NAME);
	strcpy(model_description, MODEL_OPENAI_GPT_DESCRIPTION);
	strcpy(model_url, GPT_API_URL);

	return RETURN_ZERO;
}

/*
 * Initialize AIService vtable with the function from the dall-e-2 service.
 */
static int initialize_image_generator(AIService *ai_service, char *model_name,
									  char *model_description, char *model_url)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = image_gen_initialize_service;
	ai_service->get_max_request_response_sizes =
		image_gen_get_max_request_response_sizes;
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

	/* set the model name and description */
	strcpy(model_name, MODEL_OPENAI_IMAGE_GEN_NAME);
	strcpy(model_description, MODEL_OPENAI_IMAGE_GEN_DESCRIPTION);
	strcpy(model_url, IMAGE_GEN_API_URL);

	return RETURN_ZERO;
}

/*
 * Initialize AIService vtable with functions from the embeddings service.
 */
static int initialize_embeddings(AIService *ai_service, char *model_name,
								 char *model_description, char *model_url)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = embeddings_initialize_service;
	ai_service->get_max_request_response_sizes =
		embeddings_get_max_request_response_sizes;
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

	/* set the model name and description */
	strcpy(model_name, MODEL_OPENAI_EMBEDDINGS_NAME);
	strcpy(model_description, MODEL_OPENAI_EMBEDDINGS_DESCRIPTION);
	strcpy(model_url, EMBEDDINGS_API_URL);

	return RETURN_ZERO;
}

/*
 * Initialize AIService vtable with the function from the moderation service.
 */
static int initialize_moderation(AIService *ai_service, char *model_name,
								 char *model_description, char *model_url)
{
	/* PG <-> PG_AI functions */
	ai_service->init_service_options = moderation_initialize_service;
	ai_service->get_max_request_response_sizes =
		moderation_get_max_request_response_sizes;
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

	/* set the model name and description */
	strcpy(model_name, MODEL_OPENAI_MODERATION_NAME);
	strcpy(model_description, MODEL_OPENAI_MODERATION_DESCRIPTION);
	strcpy(model_url, MODERATION_API_URL);

	return RETURN_ZERO;
}

/*
 * initializes the pointers for the supported OpenAI models
 */
int create_service_oai(size_t model_flags, AIService *ai_service,
					   char *model_name, char *model_description,
					   char *model_url)
{
	int return_value = RETURN_ERROR;

	if (model_flags & MODEL_OPENAI_GPT)
		return_value = initialize_gpt(ai_service, model_name, model_description,
									  model_url);

	if (model_flags & MODEL_OPENAI_EMBEDDINGS)
		return_value = initialize_embeddings(ai_service, model_name,
											 model_description, model_url);

	if (model_flags & MODEL_OPENAI_MODERATION)
		return_value = initialize_moderation(ai_service, model_name,
											 model_description, model_url);

	if (model_flags & MODEL_OPENAI_IMAGE_GEN)
		return_value = initialize_image_generator(ai_service, model_name,
												  model_description, model_url);

	return return_value;
}
