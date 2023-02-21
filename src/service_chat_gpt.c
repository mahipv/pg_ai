#include "service_chat_gpt.h"
#include <ctype.h>
#include <malloc.h>
#include "utils/builtins.h"
#include "postgres.h"
#include "ai_config.h"
#include "rest_transfer.h"

#define CHAT_GPT_DESCRIPTION "Open AI's text-to-text model "
#define CHAT_GPT_HELP "\nUsage: get_insights(\"chatgpt\", <column name>, '<OpenAPI Key>') \n"\
	"Usage: get_insights_agg(\"chatgpt\", <column name>, '<OpenAPI Key>') \n\n\n"

#define CHAT_GPT_API_URL "https://api.openai.com/v1/completions"
#define CHAT_GPT_SUMMARY_PROMPT "Get summary of the following in 1 lines."
#define CHAT_GPT_AGG_PROMPT "Get topic name for the words here."

#define RESPONSE_JSON_CHOICE "choices"
#define RESPONSE_JSON_KEY "text"

void
chat_gpt_name(char *name, const size_t max_len)
{
	if (name)
		strncpy(name, SERVICE_CHAT_GPT, max_len);
}

void
chat_gpt_description(char *description, const size_t max_len)
{
	if (description)
		strncpy(description, CHAT_GPT_DESCRIPTION, max_len);
}

void
chat_gpt_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, CHAT_GPT_HELP, max_len);
}

int
chat_gpt_init_service_data(void *params, void *ai_service, void *key)
{
	ServiceData		*service_data;
	char 			*column_data;

	column_data = params;
	service_data = (ServiceData*)palloc0(sizeof(ServiceData));
	service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	service_data->max_response_size = SERVICE_MAX_REQUEST_SIZE;

	strcpy(service_data->url, CHAT_GPT_API_URL);
	if (key)
		strcpy(service_data->key, key);
	else
		strcpy(service_data->key, OPEN_AI_API_KEY);
	if (((AIService *)ai_service)->is_aggregate)
		strcpy(service_data->request, CHAT_GPT_AGG_PROMPT);
	else
		strcpy(service_data->request, CHAT_GPT_SUMMARY_PROMPT);
	strcat(service_data->request, " \"");
	strcat(service_data->request, column_data);
	strcat(service_data->request, "\"");

	((AIService *)ai_service)->service_data = service_data;
	init_rest_transfer((AIService *)ai_service);
	return 0;
}

int
chat_gpt_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *)ai_service);
	return 0;
}

void
chat_gpt_set_service_buffers(RestRequest *rest_request,
							 RestResponse *rest_response,
							 ServiceData *service_data)
{
	rest_request->data = service_data->request;
	rest_request->max_size = service_data->max_request_size;

	rest_response->data = service_data->response;
	rest_response->max_size = service_data->max_response_size;
}

void
chat_gpt_rest_transfer(void *service)
{
	Datum 			choices;
	Datum 			first_choice;
	Datum 			return_text;
	AIService 		*ai_service;

	ai_service = (AIService *)(service);
	rest_transfer(ai_service);
	*((char*)(ai_service->rest_response->data) + ai_service->rest_response->data_size) = '\0';

	/*
response will be in the below format and we pick the text from the first element of
the choice array
{
  "choices": [
    {
      "text": "The capital of France is Paris.",
      "index": 0,
      "logprobs": -4.079072952270508
    }
  ]
}
*/
	if (ai_service->rest_response->response_code == HTTP_OK)
	{
		choices = DirectFunctionCall2(json_object_field_text,
									  CStringGetTextDatum(
										  (char*)(ai_service->rest_response->data)),
									  PointerGetDatum(cstring_to_text(RESPONSE_JSON_CHOICE)));
		first_choice = DirectFunctionCall2(json_array_element_text,
										   choices,
										   PointerGetDatum(0));
		return_text = DirectFunctionCall2(json_object_field_text,
										  first_choice,
										  PointerGetDatum(cstring_to_text(RESPONSE_JSON_KEY)));
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

#define CHAT_GPT_PREFIX "{\"model\":"
#define CHAT_GPT_DAVINCI_MODEL "\"text-davinci-003\", \"prompt\":\""
#define CHAT_GPT_POST_PROMPT "\", \"max_tokens\":2048}"
void
chat_gpt_post_header_maker(char *buffer, const size_t maxlen,
						   const char *data, const size_t len)
{
	strcpy(buffer, CHAT_GPT_PREFIX);
	strcat(buffer, CHAT_GPT_DAVINCI_MODEL);
	strcat(buffer, data);
	strcat(buffer, CHAT_GPT_POST_PROMPT);
}
