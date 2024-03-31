#include "ai_service_init_gem.h"

#include "services/gemini/service_gen_content.h"
#include "services/gemini/service_genc_mod.h"
#include "services/gemini/service_gen_embeddings.h"

/*
 * Initialize AIService vtable with the function from the gen_c service.
 */
static int initialize_gen_content(AIService *ai_service, char *model_name,
								  char *model_description, char *model_url)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = gen_content_initialize_service;
	ai_service->get_max_request_response_sizes =
		gen_content_get_max_request_response_sizes;
	ai_service->set_and_validate_options = gen_content_set_and_validate_options;
	ai_service->set_service_data = gen_content_set_service_data;
	ai_service->prepare_for_transfer = gen_content_prepare_for_transfer;
	ai_service->set_service_buffers = gen_content_set_service_buffers;
	ai_service->cleanup_service_data = gen_content_cleanup_service_data;
	ai_service->get_service_help = gen_content_help;

	/* PgAi <-> REST functions */
	ai_service->rest_transfer = gen_content_rest_transfer;
	ai_service->add_rest_headers = gen_content_add_rest_headers;
	ai_service->add_rest_data = gen_content_add_rest_data;

	/* set the model name and description */
	strcpy(model_name, MODEL_GEMINI_GENC_NAME);
	strcpy(model_description, MODEL_GEMINI_GENC_DESCRIPTION);
	strcpy(model_url, GENC_API_URL);

	return RETURN_ZERO;
}

/*
 * Initialize AIService vtable with the function from the gen_c service.
 */
static int initialize_genc_mod_content(AIService *ai_service, char *model_name,
									   char *model_description, char *model_url)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = genc_mod_initialize_service;
	ai_service->get_max_request_response_sizes =
		genc_mod_get_max_request_response_sizes;
	ai_service->set_and_validate_options = genc_mod_set_and_validate_options;
	ai_service->set_service_data = genc_mod_set_service_data;
	ai_service->prepare_for_transfer = genc_mod_prepare_for_transfer;
	ai_service->set_service_buffers = genc_mod_set_service_buffers;
	ai_service->cleanup_service_data = genc_mod_cleanup_service_data;
	ai_service->get_service_help = genc_mod_help;

	/* PgAi <-> REST functions */
	ai_service->rest_transfer = genc_mod_rest_transfer;
	ai_service->add_rest_headers = genc_mod_add_rest_headers;
	ai_service->add_rest_data = genc_mod_add_rest_data;

	/* set the model name and description */
	strcpy(model_name, MODEL_GEMINI_GENC_MOD_NAME);
	strcpy(model_description, MODEL_GEMINI_GENC_MOD_DESCRIPTION);
	strcpy(model_url, GENC_MOD_API_URL);

	return RETURN_ZERO;
}

/*
 * Initialize AIService vtable with the function from the embedding service.
 */
static int initialize_gen_embeddings(AIService *ai_service, char *model_name,
									 char *model_description, char *model_url)
{
	/* PG <-> PgAi functions */
	ai_service->init_service_options = gen_embeddings_initialize_service;
	ai_service->get_max_request_response_sizes =
		gen_embeddings_get_max_request_response_sizes;
	ai_service->set_and_validate_options =
		gen_embeddings_set_and_validate_options;
	ai_service->set_service_data = gen_embeddings_set_service_data;
	ai_service->prepare_for_transfer = gen_embeddings_prepare_for_transfer;
	ai_service->set_service_buffers = gen_embeddings_set_service_buffers;
	ai_service->cleanup_service_data = gen_embeddings_cleanup_service_data;
	ai_service->get_service_help = gen_embeddings_help;

	/* PgAi <-> REST functions */
	ai_service->rest_transfer = gen_embeddings_rest_transfer;
	ai_service->add_rest_headers = gen_embeddings_add_rest_headers;
	ai_service->add_rest_data = gen_embeddings_add_rest_data;

	/* set the model name and description */
	strcpy(model_name, MODEL_GEMINI_EMBEDDINGS_NAME);
	strcpy(model_description, MODEL_GEMINI_EMBEDDINGS_DESCRIPTION);
	strcpy(model_url, GEMINI_EMBEDDINGS_API_URL);

	return RETURN_ZERO;
}

/*
 * initializes the pointers for the supported OpenAI models
 */
int create_service_gem(size_t model_flags, AIService *ai_service,
					   char *model_name, char *model_description,
					   char *model_url)
{
	int return_value = RETURN_ERROR;

	if (model_flags & MODEL_GEMINI_GENC)
		return_value = initialize_gen_content(ai_service, model_name,
											  model_description, model_url);

	if (model_flags & MODEL_GEMINI_GENC_MOD)
		return_value = initialize_genc_mod_content(
			ai_service, model_name, model_description, model_url);

	if (model_flags & MODEL_GEMINI_EMBEDDINGS)
		return_value = initialize_gen_embeddings(ai_service, model_name,
												 model_description, model_url);

	return return_value;
}
