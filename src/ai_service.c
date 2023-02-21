#include "ai_service.h"
#include "service_chat_gpt.h"
#include "service_dall_e2.h"

/*
 * Clear the AIService data
 *
 * @param[out]	ai_service 	pointer to the AIService to be initialized
 * @return		void
 *
 */
void
initialize_self(AIService *ai_service)
{
	ai_service->service_data = NULL;
	ai_service->rest_request = NULL;
	ai_service->rest_response = NULL;
	ai_service->is_aggregate = 0;
}

/*
 * Initialize AIService vtable with the function from the ChatGPT service.
 *
 * @param[out]	ai_service	pointer to the AIService to be initialized
 * @return		void
 *
 */
static int
initialize_chat_gpt(AIService *ai_service)
{
	/* PG <-> AI functions */
	ai_service->get_service_help=chat_gpt_help;
	ai_service->init_service_options = chat_gpt_init_service_options;
	ai_service->init_service_data = chat_gpt_init_service_data;
	ai_service->cleanup_service_data = chat_gpt_cleanup_service_data;

	/* AI <-> REST functions */
	ai_service->set_service_buffers =  chat_gpt_set_service_buffers;
	ai_service->add_service_headers = chat_gpt_add_service_headers;
	ai_service->post_header_maker = chat_gpt_post_header_maker;
	ai_service->rest_transfer = chat_gpt_rest_transfer;

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
initialize_dalle2(AIService *ai_service)
{
	/* PG <-> AI functions */
	ai_service->get_service_help = dalle_e2_help;
	ai_service->init_service_options = dalle_e2_init_service_options;
	ai_service->init_service_data = dalle_e2_init_service_data;
	ai_service->cleanup_service_data = dalle_e2_cleanup_service_data;

	/* AI <-> REST functions */
	ai_service->set_service_buffers = dalle_e2_set_service_buffers;
	ai_service->add_service_headers = dalle_e2_add_service_headers;
	ai_service->post_header_maker = dalle_e2_post_header_maker;
	ai_service->rest_transfer = dalle_e2_rest_transfer;

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
initialize_service(const char *name, AIService *ai_service)
{
	int return_value = RETURN_ERROR;

	/* setup the service specific vtable */
	if (!strcmp(SERVICE_CHAT_GPT, name))
		return_value = initialize_chat_gpt(ai_service);

	if (!strcmp(SERVICE_DALL_E2, name))
		return_value = initialize_dalle2(ai_service);

	/* does not match any supported service */
	if (return_value == RETURN_ZERO)
	{
		/* setup common vtable */
		ai_service->get_provider_name = get_provider;
		ai_service->get_service_name = get_service_name;
		ai_service->get_service_description = get_service_description;

		/* initialize service data and define options for the service */
		(ai_service->init_service_options)(ai_service);
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
const char*
get_provider(const AIService *ai_service)
{
	return (ai_service->service_data->provider);
}

/*
 * Generic callback to return the AI service name.
 *
 * @param[in]	ai_service	pointer to the initialized AIService
 * @return		pointer too the AI service name
 *
 */
const char*
get_service_name(const AIService *ai_service)
{
	return (ai_service->service_data->name);
}

/*
 * Generic callback to return the AI service description.
 *
 * @param[in]	ai_service	Pointer to the initialized AIService
 * @return		pointer too the AI service description
 *
 */
const char*
get_service_description(const AIService *ai_service)
{
	return (ai_service->service_data->description);
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
get_service_options(const AIService *ai_service, char *text, const size_t max_len)
{
	ServiceOption	*option = ai_service->service_data->options;
	char			option_info[PG_AI_NAME_LENGTH];

	/* check service and the return ptrs */
	if (!ai_service || !text)
		return;

	while (option)
	{
		sprintf(option_info, "\n%s: %s", option->name, option->description);
		strncat(text, option_info, max_len);
		option = option->next;
	}

}
