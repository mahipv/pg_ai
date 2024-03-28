#include "service_gpt.h"

#include <utils/builtins.h>

#include "rest/rest_transfer.h"
#include "core/utils_pg_ai.h"

/*
 * Function to define aptions applicable to the gpt service calls.
 * These options are to be passed via SQL function calls or set by GUCs.
 */
static void define_options(AIService *ai_service)
{
	ServiceOption **option_list = &(ai_service->service_data->options);

	/* define the common options from the base */
	ai_service->define_common_options(ai_service);

	/* define the option to hold the column value */
	define_new_option(option_list, OPTION_COLUMN_VALUE,
					  OPTION_COLUMN_VALUE_DESC, OPTION_FLAG_HELP_DISPLAY,
					  ai_service->service_data->request,
					  SERVICE_MAX_REQUEST_SIZE);

	/* options for the non-aggregate function */
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT)
		define_new_option(option_list, OPTION_SERVICE_PROMPT,
						  OPTION_SERVICE_PROMPT_DESC,
						  OPTION_FLAG_REQUIRED | OPTION_FLAG_HELP_DISPLAY,
						  NULL /* storage ptr */, 0 /* max size */);

	/* options for the aggregate function */
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
		define_new_option(option_list, OPTION_SERVICE_PROMPT_AGG,
						  OPTION_SERVICE_PROMPT_AGG_DESC,
						  OPTION_FLAG_REQUIRED | OPTION_FLAG_HELP_DISPLAY,
						  NULL /* storage ptr */, 0 /* max size */);
}

/*
 * Initialize the options to be used for gpt. The options will hold
 * information about the AI service and some of them will be used in the curl
 * headers for REST transfer.
 */
void gpt_initialize_service(void *service)
{
	AIService *ai_service = (AIService *)service;

	/* TODO : change func signature to return error in mem context is not set */
	/* the options are stored in service data, allocate before defining */
	ai_service->service_data =
		MemoryContextAllocZero(ai_service->memory_context, sizeof(ServiceData));
	ai_service->service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	ai_service->service_data->max_response_size = SERVICE_MAX_RESPONSE_SIZE;

	/* define the options for this service - stored in service data */
	define_options(ai_service);
}

/*
 * Return the help text to be displayed for the gpt service
 */
void gpt_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, GPT_HELP, max_len);
}

/*
 * Function to set the options for the gpt service. The options are set from
 * the function arguments or from the GUCs. The options are validated and
 * the function returns an error if any of the required options are not set.
 */
int gpt_set_and_validate_options(void *service, void *function_options)
{
	PG_FUNCTION_ARGS = (FunctionCallInfo)function_options;
	AIService *ai_service = (AIService *)service;
	ServiceOption *options = ai_service->service_data->options;
	int arg_offset;

	/* aggregate functions get an extra argument at position 0 */
	arg_offset =
		(ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE) ? 1 : 0;

	/* set the default prompt or passed prompt based on the function */
	if (!PG_ARGISNULL(1 + arg_offset))
	{
		if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
			set_option_value(options, OPTION_SERVICE_PROMPT_AGG,
							 text_to_cstring(PG_GETARG_TEXT_P(1 + arg_offset)),
							 false /* concat */);
		else
			set_option_value(options, OPTION_SERVICE_PROMPT,
							 text_to_cstring(PG_GETARG_TEXT_P(1 + arg_offset)),
							 false /* concat */);
	}
	else /* set the default prompts */
	{
		if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
			set_option_value(options, OPTION_SERVICE_PROMPT_AGG, GPT_AGG_PROMPT,
							 false /* concat */);
		else
			set_option_value(options, OPTION_SERVICE_PROMPT, GPT_SUMMARY_PROMPT,
							 false /* concat */);
	}

	/* check if all required options are set */
	for (options = ai_service->service_data->options; options;
		 options = options->next)
	{
		if ((options->flags & OPTION_FLAG_REQUIRED) &&
			!(options->flags & OPTION_FLAG_IS_SET))
		{
			ereport(INFO, (errmsg("Required %s option \"%s\" missing.\n",
								  options->flags & OPTION_FLAG_GUC ? "GUC" :
																	 "function",
								  options->name)));
			return RETURN_ERROR;
		}
	}
	return RETURN_ZERO;
}

/*
 * Function to set the service data for the service.
 */
int gpt_set_service_data(void *service, void *data)
{
	AIService *ai_service = (AIService *)service;
	bool concat = false;

	/* if aggregate function then concat */
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
		concat = true;

	/* set or concat the column data to be used in the request */
	set_option_value(ai_service->service_data->options, OPTION_COLUMN_VALUE,
					 (char *)data, concat);

	/* ereport(INFO, (errmsg("Data set: %s\n", (char *)data))); */
	return RETURN_ZERO;
}

/*
 * Function to prepare the service data for transfer. This function is called
 * from the PG layer before the REST transfer is initiated. The function
 * prepares the request data and sets the transfer options.
 */
int gpt_prepare_for_transfer(void *service)
{
	AIService *ai_service = (AIService *)service;
	ServiceOption *option_list = ai_service->service_data->options;
	char prompt[SERVICE_DATA_SIZE];
	size_t prompt_len;
	ServiceOption *option;

	/* set the prompt based on the function */
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT)
		snprintf(prompt, SERVICE_DATA_SIZE, "%s : \"",
				 get_option_value(option_list, OPTION_SERVICE_PROMPT));

	if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
		snprintf(prompt, SERVICE_DATA_SIZE, "%s : \"",
				 get_option_value(option_list, OPTION_SERVICE_PROMPT_AGG));

	/* check if the column values is too big */
	prompt_len = strlen(prompt);
	option = get_option(option_list, OPTION_COLUMN_VALUE);
	if (!option)
		ereport(ERROR, (errmsg("Column value option not set.")));

	/* make sure there is enough space for prompt */
	if (option->current_len + prompt_len + 2 > option->max_len)
		ereport(ERROR, (errmsg("Column value is too big.")));

	/* move data to include the prompt and close with a '"' */
	memmove(option->value_ptr + prompt_len, option->value_ptr,
			option->current_len);
	memcpy(option->value_ptr, prompt, prompt_len);
	option->value_ptr[option->current_len + prompt_len] = '"';
	option->value_ptr[option->current_len + prompt_len + 1] = '\0';

	/* ereport(INFO,(errmsg("Req: %s\n",ai_service->service_data->request))); */
	init_rest_transfer((AIService *)ai_service);

	return RETURN_ZERO;
}

/*
 * Function to cleanup the transfer structures before initiating a new
 * transfer request.
 */
int gpt_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *)ai_service);
	return RETURN_ZERO;
}

/*
 * Function to initialize the service buffers for data tranasfer.
 */
void gpt_set_service_buffers(RestRequest *rest_request,
							 RestResponse *rest_response,
							 ServiceData *service_data)
{
	rest_request->data = service_data->request;
	rest_request->max_size = service_data->max_request_size;

	rest_response->data = service_data->response;
	rest_response->max_size = service_data->max_response_size;
}

/*
 * Initialize the service headers in the curl context. This is a
 * call back from REST transfer layer. The curl headers are
 * constructed from this list.
 */
int gpt_add_rest_headers(CURL *curl, struct curl_slist **headers, void *service)
{
	AIService *ai_service = (AIService *)service;
	struct curl_slist *curl_headers = *headers;
	char key_header[128];

	curl_headers =
		curl_slist_append(curl_headers, "Content-Type: application/json");
	snprintf(key_header, sizeof(key_header), "Authorization: Bearer %s",
			 get_option_value(ai_service->service_data->options,
							  OPTION_SERVICE_API_KEY));
	curl_headers = curl_slist_append(curl_headers, key_header);
	*headers = curl_headers;

	return RETURN_ZERO;
}

/* TODO the token count should be dynamic */
/*
 * Callback to make the post header
 *
 */
#define GPT_MODEL_KEY "model"
#define GPT_PROMPT_KEY "prompt"
#define GPT_MAX_TOKENS_KEY "max_tokens"
#define GPT_MAX_TOKENS_VAUE "1024"
#define JSON_DATA_TYPE_STRING "string"
#define JSON_DATA_TYPE_INTEGER "integer"

void gpt_add_rest_data(char *buffer, const size_t maxlen, const char *data,
					   const size_t len)
{
	const char *data_types[] = {JSON_DATA_TYPE_STRING, JSON_DATA_TYPE_STRING,
								JSON_DATA_TYPE_INTEGER};
	const char *keys[] = {GPT_MODEL_KEY, GPT_PROMPT_KEY, GPT_MAX_TOKENS_KEY};
	const char *values[] = {MODEL_OPENAI_GPT_NAME, data, GPT_MAX_TOKENS_VAUE};

	generate_json(buffer, keys, values, data_types, 3);
}

/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 */
void gpt_rest_transfer(void *service)
{
	Datum choices;
	Datum first_choice;
	Datum return_text;
	AIService *ai_service;

	ai_service = (AIService *)(service);
	rest_transfer(ai_service);
	*((char *)(ai_service->rest_response->data) +
	  ai_service->rest_response->data_size) = '\0';

	/*
	response will be in the below format and we pick the text from the first
	element of the choice array
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
		choices = DirectFunctionCall2(
			json_object_field_text,
			CStringGetTextDatum((char *)(ai_service->rest_response->data)),
			PointerGetDatum(cstring_to_text(RESPONSE_JSON_CHOICE)));
		first_choice = DirectFunctionCall2(json_array_element_text, choices,
										   PointerGetDatum(0));
		return_text = DirectFunctionCall2(
			json_object_field_text, first_choice,
			PointerGetDatum(cstring_to_text(RESPONSE_JSON_KEY)));
		strcpy(ai_service->service_data->response,
			   text_to_cstring(DatumGetTextPP(return_text)));
	}
	else if (ai_service->rest_response->data_size == 0)
	{
		strcpy(ai_service->service_data->response,
			   "Something is not ok, try again.");
	}

	/* remove prefexing \n */
	for (int i = 0; ai_service->service_data->response[i] != '\0'; i++)
		if (ai_service->service_data->response[i] == '\n')
			ai_service->service_data->response[i] = ' ';
		else
			break;
}
