#include "ai_service_init_gem.h"

#include "services/gemini/service_gen_content.h"

/*
 * Initialize AIService vtable with the function from the ChatGPT service.
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

	return return_value;
}
