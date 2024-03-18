#include "ai_service.h"
#include <funcapi.h>
#include "guc/pg_ai_guc.h"
#include "services/openai/service_gpt.h"
#include "services/openai/service_image_gen.h"
#include "services/openai/service_embeddings.h"
#include "services/openai/service_moderation.h"

/*
 * Clear the AIService data
 *
 * @param[out]	ai_service 	pointer to the AIService to be initialized
 * @return		void
 *
 */
void
reset_service(AIService * ai_service)
{
	if (ai_service->memory_context)
		MemoryContextReset(ai_service->memory_context);
}

/*
 * Initialize AIService vtable with the function from the ChatGPT service.
 *
 * @param[out]	ai_service	pointer to the AIService to be initialized
 * @return		void
 *
 */
static int
initialize_gpt(AIService * ai_service)
{
	/* PG <-> PG_AI functions */
	ai_service->get_service_help = gpt_help;
	ai_service->init_service_options = gpt_init_service_options;
	ai_service->set_and_validate_options = gpt_set_and_validate_options;
	ai_service->init_service_data = gpt_init_service_data;
	ai_service->cleanup_service_data = gpt_cleanup_service_data;

	/* PG_AI <-> REST functions */
	ai_service->set_service_buffers = gpt_set_service_buffers;
	ai_service->add_service_headers = gpt_add_service_headers;
	ai_service->post_header_maker = gpt_post_header_maker;
	ai_service->rest_transfer = gpt_rest_transfer;

	return 0;
}

/*
 * Initialize AIService vtable with the function from the Dalle-2 service.
 *
 * @param[out]	ai_service	pointer to the AIService to be initialized
 * @return:		zero on success, non-zero otherwise
 *
 */
static int
initialize_image_generator(AIService * ai_service)
{
	/* PG <-> PG_AI functions */
	ai_service->get_service_help = image_gen_help;
	ai_service->init_service_options = image_gen_init_service_options;
	ai_service->set_and_validate_options = image_gen_set_and_validate_options;
	ai_service->init_service_data = image_gen_init_service_data;
	ai_service->cleanup_service_data = image_gen_cleanup_service_data;

	/* PG_AI <-> REST functions */
	ai_service->set_service_buffers = image_gen_set_service_buffers;
	ai_service->add_service_headers = image_gen_add_service_headers;
	ai_service->post_header_maker = image_gen_post_header_maker;
	ai_service->rest_transfer = image_gen_rest_transfer;

	return 0;
}

/*
 * Initialize AIService vtable with the function from the ChatGPT service.
 *
 * @param[out]	ai_service	pointer to the AIService to be initialized
 * @return		void
 *
 */
static int
initialize_embeddings(AIService * ai_service)
{
	/* PG <-> PG_AI functions */
	ai_service->get_service_help = embeddings_help;
	ai_service->init_service_options = embeddings_init_service_options;
	ai_service->set_and_validate_options = embeddings_set_and_validate_options;
	ai_service->init_service_data = embeddings_init_service_data;
	ai_service->cleanup_service_data = embeddings_cleanup_service_data;

	/* PG_AI <-> REST functions */
	ai_service->set_service_buffers = embeddings_set_service_buffers;
	ai_service->add_service_headers = embeddings_add_service_headers;
	ai_service->post_header_maker = embeddings_post_header_maker;
	ai_service->rest_transfer = embeddings_rest_transfer;
	return 0;
}

/*
 * Initialize AIService vtable with the function from the ChatGPT service.
 *
 * @param[out]	ai_service	pointer to the AIService to be initialized
 * @return		void
 *
 */
static int
initialize_moderation(AIService * ai_service)
{
	/* PG <-> PG_AI functions */
	ai_service->get_service_help = moderation_help;
	ai_service->init_service_options = moderation_init_service_options;
	ai_service->set_and_validate_options = moderation_set_and_validate_options;
	ai_service->init_service_data = moderation_init_service_data;
	ai_service->cleanup_service_data = moderation_cleanup_service_data;

	/* PG_AI <-> REST functions */
	ai_service->set_service_buffers = moderation_set_service_buffers;
	ai_service->add_service_headers = moderation_add_service_headers;
	ai_service->post_header_maker = moderation_post_header_maker;
	ai_service->rest_transfer = moderation_rest_transfer;

	return 0;
}


/*
 * Initialize the AIService based on the service parameter.
 *
 * @param[in/out]	ai_service	pointer to the AIService to be initialized
 * @param[in] 		name		pointer to the service name
 * @return			zero on success, non-zero otherwise
 *
 */
int
initialize_service(const int service_flags, const int model_flags, AIService * ai_service)
{
	int			return_value = RETURN_ERROR;
	char		service_name[PG_AI_NAME_LENGTH];
	char		service_description[PG_AI_DESC_LENGTH];
	char		model_name[PG_AI_NAME_LENGTH];
	char		model_description[PG_AI_DESC_LENGTH];
	char		model_url[SERVICE_DATA_SIZE];
	char	   *api_key;

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
		/* initialize service data and define options for the service */
		(ai_service->init_service_options) (ai_service);

		/* set the constant model specific info and options */
		strcpy(ai_service->service_data->service_description, service_description);
		strcpy(ai_service->service_data->model_description, model_description);
		set_option_value(ai_service->service_data->options, OPTION_SERVICE_NAME, service_name);
		set_option_value(ai_service->service_data->options, OPTION_MODEL_NAME, model_name);
		set_option_value(ai_service->service_data->options, OPTION_ENDPOINT_URL, model_url);
		api_key = get_pg_ai_guc_string_variable(PG_AI_GUC_API_KEY);
		if (api_key)
			set_option_value(ai_service->service_data->options, OPTION_SERVICE_API_KEY, api_key);
	}
	return return_value;
}

/*
 * Generic callback to return the AI service's provider.
 *
 * @param[in]	ai_service	pointer to the initialized AIService
 * @return		pointer too the AI service provider
 *
 */
const char *
get_service_name(const AIService * ai_service)
{
	return (get_option_value(ai_service->service_data->options, OPTION_SERVICE_NAME));
}

/*
 * Generic callback to return the AI service name.
 *
 * @param[in]	ai_service	pointer to the initialized AIService
 * @return		pointer too the AI service name
 *
 */
const char *
get_model_name(const AIService * ai_service)
{
	return (get_option_value(ai_service->service_data->options, OPTION_MODEL_NAME));
}

const char *
get_service_description(const AIService * ai_service)
{
	return (ai_service->service_data->service_description);
}

const char *
get_model_description(const AIService * ai_service)
{
	return (ai_service->service_data->model_description);
}

/*
 * Generic callback to return the parameters applicable to a AI service.
 * This method assumes that the AIService is initialized, which defines
 * the parameters for the service.
 *
 * @param[in]	ai_service	pointer to the initialized AIService
 * @param[out]	text		pointer in which parameter list is returned
 * @param[in]	max_len		max length the text parameter can accomodate
 * @return		void
 *
 */
void
get_service_options(const AIService * ai_service, char *text, const size_t max_len)
{
	ServiceOption *option = ai_service->service_data->options;
	char		option_info[PG_AI_NAME_LENGTH];

	/* check service and the return ptrs */
	if (!ai_service || !text)
		return;

	while (option)
	{
		if (option->help_display)
		{
			sprintf(option_info, "\n%s: %s", option->name, option->description);
			strncat(text, option_info, max_len);
		}
		option = option->next;
	}
}
