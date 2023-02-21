#include "service_dalle2.h"
#include <ctype.h>
#include <malloc.h>
#include "postgres.h"
#include "utils/builtins.h"
#include "rest_transfer.h"
#include "ai_config.h"

#define DALLE2_DESCRIPTION "Open AI's text-to-image model "
#define DALLE2_HELP "\nUsage: get_insights(\"dalle-2\", <column name>, '<OpenAPI Key>') \n"\
	"Usage: get_insights_agg(\"dalle-2\", <column name>, '<OpenAPI Key>') \n\n\n"

#define DALLE2_API_URL "https://api.openai.com/v1/images/generations"
#define DALLE2_SUMMARY_PROMPT "Make a picture with the following"
#define DALLE2_AGG_PROMPT "Make a picture with the following "

#define RESPONSE_JSON_DATA "data"
#define RESPONSE_JSON_URL "url"

void
dalle2_name(char *name, const size_t max_len)
{
	if (name)
		strncpy(name, SERVICE_DALLE_2, max_len);
}

void
dalle2_description(char *description, const size_t max_len)
{
	if (description)
		strncpy(description, DALLE2_DESCRIPTION, max_len);
}

void
dalle2_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, DALLE2_HELP, max_len);
}

int
dalle2_init_service_data(void *params, void *ai_service, void *key)
{
	ServiceData		*service_data;
	char 			*column_data;

	column_data = params;
	service_data = (ServiceData*)palloc0(sizeof(ServiceData));
	service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	service_data->max_response_size = SERVICE_MAX_REQUEST_SIZE;

	strcpy(service_data->url, DALLE2_API_URL);
	if (key)
		strcpy(service_data->key, key);
	else
		strcpy(service_data->key, OPEN_AI_API_KEY);
	if (((AIService *)ai_service)->is_aggregate)
		strcpy(service_data->request, DALLE2_AGG_PROMPT);
	else
		strcpy(service_data->request, DALLE2_SUMMARY_PROMPT);
	strcat(service_data->request, " \"");
	strcat(service_data->request, column_data);
	strcat(service_data->request, "\"");

	((AIService *)ai_service)->service_data = service_data;
	init_rest_transfer((AIService *)ai_service);
	return 0;
}

int
dalle2_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *)ai_service);
	return 0;
}

void
dalle2_set_service_buffers(RestRequest *rest_request,
						   RestResponse *rest_response,
						   ServiceData *service_data)
{
	rest_request->data = service_data->request;
	rest_request->max_size = service_data->max_request_size;

	rest_response->data = service_data->response;
	rest_response->max_size = service_data->max_response_size;
}

void
dalle2_rest_transfer(void *service)
{
	Datum 			data;
	Datum 			url;
	Datum 			return_text;
	AIService 		*ai_service;

	ai_service = (AIService *)(service);
	rest_transfer(ai_service);
	*((char*)(ai_service->rest_response->data) + ai_service->rest_response->data_size) = '\0';

	if (ai_service->rest_response->response_code == HTTP_OK)
	{
		data = DirectFunctionCall2(json_object_field_text,
								   CStringGetTextDatum(
									   (char*)(ai_service->rest_response->data)),
								   PointerGetDatum(cstring_to_text(RESPONSE_JSON_DATA)));
		url = DirectFunctionCall2(json_array_element_text,
								  data,
								  PointerGetDatum(0));
		return_text = DirectFunctionCall2(json_object_field_text,
										  url,
										  PointerGetDatum(cstring_to_text(RESPONSE_JSON_URL)));

		strcpy(ai_service->service_data->response,
			   text_to_cstring(DatumGetTextPP(return_text)));
	}
	else if (ai_service->rest_response->data_size == 0)
	{
		strcpy(ai_service->service_data->response, "Something is not ok, try again.");
	}
	/* remove prefexing \n */
	for (int i = 0; ai_service->service_data->response[i] != '\0'; i++)
		if (ai_service->service_data->response[i] != '\n')
			break;
		else
			ai_service->service_data->response[i] = ' ';
}

/* TODO */
#define DALLE2_PRE_PREFIX "{\"prompt\":\""
#define DALLE2_POST_PREFIX "\",\"num_images\":1,\"size\":\"1024x1024\",\"response_format\":\"url\"}"
void
dalle2_post_header_maker(char *buffer, const size_t maxlen,
						 const char *data, const size_t len)
{
	strcpy(buffer, DALLE2_PRE_PREFIX);
	strcat(buffer, data);
	strcat(buffer, DALLE2_POST_PREFIX);
}
