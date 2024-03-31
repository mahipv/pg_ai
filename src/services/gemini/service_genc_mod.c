#include "service_genc_mod.h"

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
					  ai_service->service_data->request_data,
					  SERVICE_MAX_REQUEST_SIZE);

	/* options for the non-aggregate function */
	if (ai_service->function_flags & FUNCTION_MODERATION)
		define_new_option(option_list, OPTION_SERVICE_PROMPT,
						  OPTION_SERVICE_PROMPT_DESC,
						  OPTION_FLAG_REQUIRED | OPTION_FLAG_HELP_DISPLAY,
						  NULL /* storage ptr */, 0 /* max size */);

	/* options for the aggregate function */
	if (ai_service->function_flags & FUNCTION_MODERATION_AGGREGATE)
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
void genc_mod_initialize_service(void *service)
{
	AIService *ai_service = (AIService *)service;

	/* define the options for this service - stored in service data */
	define_options(ai_service);
}

/*
 * Return the help text to be displayed for the gpt service
 */
void genc_mod_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, GEN_CONTENT_HELP, max_len);
}

/*
 * Function to set the options for the gpt service. The options are set from
 * the function arguments or from the GUCs. The options are validated and
 * the function returns an error if any of the required options are not set.
 */
int genc_mod_set_and_validate_options(void *service, void *function_options)
{
	PG_FUNCTION_ARGS = (FunctionCallInfo)function_options;
	AIService *ai_service = (AIService *)service;
	ServiceOption *options = ai_service->service_data->options;
	int arg_offset;
	char temp_url[SERVICE_DATA_SIZE];
	char *url;

	/* aggregate functions get an extra argument at position 0 */
	arg_offset =
		(ai_service->function_flags & FUNCTION_MODERATION_AGGREGATE) ? 1 : 0;

	/* set the default prompt or passed prompt based on the function */
	if (!PG_ARGISNULL(1 + arg_offset))
	{
		if (ai_service->function_flags & FUNCTION_MODERATION_AGGREGATE)
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
		if (ai_service->function_flags & FUNCTION_MODERATION_AGGREGATE)
			set_option_value(options, OPTION_SERVICE_PROMPT_AGG,
							 GENC_MOD_AGG_PROMPT, false /* concat */);
		else
			set_option_value(options, OPTION_SERVICE_PROMPT,
							 GENC_MOD_SUMMARY_PROMPT, false /* concat */);
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

	/* manipulate the url */
	url = get_option_value(ai_service->service_data->options,
						   OPTION_ENDPOINT_URL);
	strcpy(temp_url, url);
	strcat(temp_url, get_option_value(ai_service->service_data->options,
									  OPTION_SERVICE_API_KEY));
	set_option_value(ai_service->service_data->options, OPTION_ENDPOINT_URL,
					 temp_url, false /* concat */);

	return RETURN_ZERO;
}

/*
 * Function to set the service data for the service.
 */
int genc_mod_set_service_data(void *service, void *data)
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
int genc_mod_prepare_for_transfer(void *service)
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
int genc_mod_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *)ai_service);
	return RETURN_ZERO;
}

/*
 * Function to initialize the service buffers for data tranasfer.
 */
void genc_mod_set_service_buffers(RestRequest *rest_request,
								  RestResponse *rest_response,
								  ServiceData *service_data)
{
	rest_request->data = service_data->request_data;
	rest_request->max_size = service_data->max_request_size;

	rest_response->data = service_data->response_data;
	rest_response->max_size = service_data->max_response_size;
}

/*
 * Initialize the service headers in the curl context. This is a
 * call back from REST transfer layer. The curl headers are
 * constructed from this list.
 */
int genc_mod_add_rest_headers(CURL *curl, struct curl_slist **headers,
							  void *service)
{
	struct curl_slist *curl_headers = *headers;

	curl_headers =
		curl_slist_append(curl_headers, "Content-Type: application/json");
	*headers = curl_headers;

	return RETURN_ZERO;
}

/*
 * Callback to make the post header. This is for content moderation so set the
 * expected out put to minimal.
 */
#define JSON_FORMAT_STR                                                        \
	"{\n"                                                                      \
	"  \"contents\": [{\n"                                                     \
	"    \"parts\":[{\n"                                                       \
	"      \"text\": \"%s\"\n"                                                 \
	"    }]\n"                                                                 \
	"  }],\n"                                                                  \
	"   \"generationConfig\": {\n"                                             \
	"   \"candidateCount\": 1, \n"                                             \
	"   \"maxOutputTokens\": 1 \n"                                             \
	"  }\n"                                                                    \
	"}"

void genc_mod_add_rest_data(char *buffer, const size_t maxlen, const char *data,
							const size_t len)
{
	sprintf(buffer, JSON_FORMAT_STR, data);
	// ereport(INFO, (errmsg("Request: %s\n", buffer)));
}

/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 */
#define RESPONSE_JSON_PROMPTFEEDBACK "promptFeedback"
void genc_mod_rest_transfer(void *service)
{
	Datum prompt_feedback;
	AIService *ai_service;

	ai_service = (AIService *)(service);
	rest_transfer(ai_service);
	*((char *)(ai_service->rest_response->data) +
	  ai_service->rest_response->data_size) = '\0';

	/*
		ereport(INFO, (errmsg("Response: %s\n",
						  (char *)ai_service->rest_response->data)));
	*/

	/*
	 * Refer https://ai.google.dev/tutorials/rest_quickstart for response
	 * format
	 */
	if (ai_service->rest_response->response_code == HTTP_OK)
	{
		/* get content of the first candidate */
		prompt_feedback = DirectFunctionCall2(
			json_object_field_text,
			CStringGetTextDatum((char *)(ai_service->rest_response->data)),
			PointerGetDatum(cstring_to_text(RESPONSE_JSON_PROMPTFEEDBACK)));
		strcpy(ai_service->service_data->response_data,
			   text_to_cstring(DatumGetTextPP(prompt_feedback)));
	}
	else if (ai_service->rest_response->data_size == 0)
	{
		strcpy(ai_service->service_data->response_data,
			   "Something is not ok, try again.");
	}

	/* remove prefexing \n */
	for (int i = 0; ai_service->service_data->response_data[i] != '\0'; i++)
		if (ai_service->service_data->response_data[i] == '\n')
			ai_service->service_data->response_data[i] = ' ';
		else
			break;
}

/* this has to be based on the context lengths of the supported services */
void genc_mod_get_max_request_response_sizes(size_t *max_request_size,
											 size_t *max_response_size)
{
	*max_request_size = SERVICE_MAX_REQUEST_SIZE;
	*max_response_size = SERVICE_MAX_RESPONSE_SIZE;
}