#include "ai_service.h"
#include "service_chat_gpt.h"
#include "service_dalle2.h"

void
initialize_self(AIService *ai_service)
{
	ai_service->service_data = NULL;
	ai_service->rest_request = NULL;
	ai_service->rest_response = NULL;
	ai_service->is_aggregate = 0;
}

static int
initialize_chat_gpt(AIService *ai_service)
{
	/*
	   if needed this can be used to check if the get_service_name
	   returns the same value. If not, the get_service_name is
	   is changed and the this ai_service is invalid.
	*/
	strcpy(ai_service->name, SERVICE_CHAT_GPT);
	ai_service->get_service_name = chat_gpt_name;
	ai_service->get_service_description = chat_gpt_description;
	ai_service->get_service_help=chat_gpt_help;
	ai_service->init_service_data = chat_gpt_init_service_data;
	ai_service->cleanup_service_data = chat_gpt_cleanup_service_data;

	ai_service->set_service_buffers =  chat_gpt_set_service_buffers;
	ai_service->post_header_maker = chat_gpt_post_header_maker;
	ai_service->rest_transfer = chat_gpt_rest_transfer;

	return 0;
}

static int
initialize_dalle2(AIService *ai_service)
{
	/*
	   if needed this can be used to check if the get_service_name
	   returns the same value. If not, the get_service_name is
	   is changed and the this ai_service is invalid.
	*/
	strcpy(ai_service->name, SERVICE_CHAT_GPT);
	ai_service->get_service_name = dalle2_name;
	ai_service->get_service_description = dalle2_description;
	ai_service->get_service_help = dalle2_help;
	ai_service->init_service_data = dalle2_init_service_data;
	ai_service->cleanup_service_data = dalle2_cleanup_service_data;

	ai_service->set_service_buffers = dalle2_set_service_buffers;
	ai_service->post_header_maker = dalle2_post_header_maker;
	ai_service->rest_transfer = dalle2_rest_transfer;

	return 0;
}

int
initialize_service(const char *name, AIService *ai_service)
{
	int return_value = 1;

	if (!strcmp(SERVICE_CHAT_GPT, name))
		return initialize_chat_gpt(ai_service);

	if (!strcmp(SERVICE_DALLE_2, name))
		return initialize_dalle2(ai_service);

	return return_value;
}
