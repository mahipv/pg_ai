#include "ai_service.h"
#include "openai_services/service_gpt.h"
#include "openai_services/service_image_gen.h"
#include "openai_services/service_embeddings.h"
#include "openai_services/service_moderation.h"

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
	memset(ai_service, 0, sizeof(AIService));
}

/*
 * Initialize AIService vtable with the function from the ChatGPT service.
 *
 * @param[out]	ai_service	pointer to the AIService to be initialized
 * @return		void
 *
 */
static int
initialize_chat_gpt(AIService * ai_service)
{
	/* PG <-> AI functions */
	ai_service->get_service_help = gpt_help;
	ai_service->init_service_options = gpt_init_service_options;
	ai_service->set_and_validate_options = gpt_set_and_validate_options;
	ai_service->init_service_data = gpt_init_service_data;
	ai_service->cleanup_service_data = gpt_cleanup_service_data;

	/* AI <-> REST functions */
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
initialize_dalle2(AIService * ai_service)
{
	/* PG <-> AI functions */
	ai_service->get_service_help = image_gen_help;
	ai_service->init_service_options = image_gen_init_service_options;
	ai_service->set_and_validate_options = image_gen_set_and_validate_options;
	ai_service->init_service_data = image_gen_init_service_data;
	ai_service->cleanup_service_data = image_gen_cleanup_service_data;

	/* AI <-> REST functions */
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
initialize_ada(AIService * ai_service)
{
	/* PG <-> AI functions */
	ai_service->get_service_help = embeddings_help;
	ai_service->init_service_options = embeddings_init_service_options;
	ai_service->set_and_validate_options = embeddings_set_and_validate_options;
	ai_service->init_service_data = embeddings_init_service_data;
	ai_service->cleanup_service_data = embeddings_cleanup_service_data;

	/* AI <-> REST functions */
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
	/* PG <-> AI functions */
	ai_service->get_service_help = moderation_help;
	ai_service->init_service_options = moderation_init_service_options;
	ai_service->set_and_validate_options = moderation_set_and_validate_options;
	ai_service->init_service_data = moderation_init_service_data;
	ai_service->cleanup_service_data = moderation_cleanup_service_data;

	/* AI <-> REST functions */
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
initialize_service(const char *service_name, char *model_name, AIService * ai_service)
{
	int			return_value = RETURN_ERROR;

	if (!strcmp(SERVICE_OPENAI, service_name))
	{
		if (!strcmp(MODEL_OPENAI_GPT, model_name))
			return_value = initialize_chat_gpt(ai_service);

		if (!strcmp(MODEL_OPENAI_IMAGE_GEN, model_name))
			return_value = initialize_dalle2(ai_service);

		if (!strcmp(MODEL_OPENAI_EMBEDDINGS, model_name))
			return_value = initialize_ada(ai_service);

		if (!strcmp(MODEL_OPENAI_MODERATION, model_name))
			return_value = initialize_moderation(ai_service);
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
	return (ai_service->service_data->name);
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
	return (ai_service->service_data->model);
}

/*
 * Generic callback to return the AI service description.
 *
 * @param[in]	ai_service	Pointer to the initialized AIService
 * @return		pointer too the AI service description
 *
 */
const char *
get_service_description(const AIService * ai_service)
{
	return (ai_service->service_data->name_description);
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
