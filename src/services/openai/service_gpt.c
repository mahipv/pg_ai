#include "service_gpt.h"

#include "utils/builtins.h"

#include "rest/rest_transfer.h"
#include "utils_pg_ai.h"


/*
* Function to define aptions applicable to the gpt service calls.
* These options are to be passed via SQL function calls or set by GUCs.
*/
static void
define_options(AIService * ai_service)
{
	ServiceOption **option_list = &(ai_service->service_data->options);

	/* common options for the services */
	define_new_option(option_list, OPTION_SERVICE_NAME,
					  OPTION_SERVICE_NAME_DESC, true /* guc */ ,
					  true /* required */ , false /* help_display */ );
	define_new_option(option_list, OPTION_MODEL_NAME,
					  OPTION_MODEL_NAME_DESC, true /* guc */ ,
					  true /* required */ , false /* help_display */ );
	define_new_option(option_list, OPTION_ENDPOINT_URL,
					  OPTION_ENDPOINT_URL_DESC, true /* guc */ ,
					  true /* required */ , false /* help_display */ );
	define_new_option(option_list, OPTION_SERVICE_API_KEY,
					  OPTION_SERVICE_API_KEY_DESC, true /* guc */ ,
					  true /* required */ , false /* help_display */ );
	define_new_option(option_list, OPTION_COLUMN_VALUE,
					  OPTION_COLUMN_VALUE_DESC, false /* guc */ ,
					  false /* required */ , true /* help_display */ );

	/* options for the non-aggregate function */
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT)
	{
		define_new_option(option_list, OPTION_SERVICE_PROMPT,
						  OPTION_SERVICE_PROMPT_DESC, false /* guc */ ,
						  true /* required */ , true /* help_display */ );
	}

	/* options for the aggregate function */
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
		define_new_option(option_list, OPTION_SERVICE_PROMPT_AGG,
						  OPTION_SERVICE_PROMPT_AGG_DESC, false /* guc */ ,
						  true /* required */ , true /* help_display */ );
}


/*
 * Initialize the options to be used for gpt. The options will hold
 * information about the AI service and some of them will be used in the curl
 * headers for REST transfer.
 */
void
gpt_init_service_options(void *service)
{
	AIService  *ai_service = (AIService *) service;
	ServiceData *service_data;

	service_data = (ServiceData *) palloc0(sizeof(ServiceData));
	ai_service->service_data = service_data;

	/* Define the options for this service */
	define_options(ai_service);
}


/*
 * Return the help text to be displayed for the gpt service
 */
void
gpt_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, GPT_HELP, max_len);
}


/*
* Function to set the options for the gpt service. The options are set from
* the function arguments or from the GUCs. The options are validated and
* the function returns an error if any of the required options are not set.
*/
int
gpt_set_and_validate_options(void *service, void *function_options)
{
	PG_FUNCTION_ARGS = (FunctionCallInfo) function_options;
	AIService  *ai_service = (AIService *) service;
	ServiceOption *options = ai_service->service_data->options;
	int			arg_offset;

	/* aggregate functions get an extra argument at position 0 */
	arg_offset = (ai_service->function_flags &
				  FUNCTION_GET_INSIGHT_AGGREGATE) ? 1 : 0;

	if ((ai_service->function_flags & FUNCTION_GET_INSIGHT) &&
		(!PG_ARGISNULL(0 + arg_offset)))
		set_option_value(options, OPTION_COLUMN_VALUE,
						 text_to_cstring(PG_GETARG_TEXT_P(0 + arg_offset)),
						 NULL /* value_ptr */ , 0 /* size */ );

	/* set the default prompt or passed prompt based on the function */
	if (!PG_ARGISNULL(1 + arg_offset))
	{
		if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
			set_option_value(options, OPTION_SERVICE_PROMPT_AGG,
							 text_to_cstring(PG_GETARG_TEXT_P(1 + arg_offset)),
							 NULL /* value_ptr */ , 0 /* size */ );
		else
			set_option_value(options, OPTION_SERVICE_PROMPT,
							 text_to_cstring(PG_GETARG_TEXT_P(1 + arg_offset)),
							 NULL /* value_ptr */ , 0 /* size */ );
	}
	else						/* set the default prompts */
	{
		if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
			set_option_value(options, OPTION_SERVICE_PROMPT_AGG,
							 GPT_AGG_PROMPT, NULL /* value_ptr */ , 0 /* size */ );
		else
			set_option_value(options, OPTION_SERVICE_PROMPT,
							 GPT_SUMMARY_PROMPT, NULL /* value_ptr */ , 0 /* size */ );
	}

	/* check if all required options are set */
	for (options = ai_service->service_data->options; options;
		 options = options->next)
	{
		if (options->required && !options->is_set)
		{
			ereport(INFO, (errmsg("Required %s option \"%s\" missing.\n",
								  options->guc_option ? "GUC" : "function", options->name)));
			return RETURN_ERROR;
		}
	}
	return RETURN_ZERO;
}


/*
 * This function is called from the PG layer after it has the table data and
 * before invoking the REST transfer call. Load the json options(will be used
 * for the transfer)and copy the data received from PG into the REST request
 * structures.
 */
int
gpt_init_service_data(void *options, void *service, void *data)
{
	ServiceData *service_data;
	char	   *column_data;
	AIService  *ai_service = (AIService *) service;

	service_data = ai_service->service_data;
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT)
		column_data = get_option_value(ai_service->service_data->options, OPTION_COLUMN_VALUE);
	else
		column_data = (char *) data;

	service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	service_data->max_response_size = SERVICE_MAX_RESPONSE_SIZE;

	if (ai_service->function_flags & FUNCTION_GET_INSIGHT)
		strcpy(service_data->request,
			   get_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT));

	if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
		strcpy(service_data->request,
			   get_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT_AGG));

	strcat(service_data->request, " \"");
	strcat(service_data->request, column_data);
	strcat(service_data->request, "\"");

	/* ereport(INFO,(errmsg("Request: %s\n", service_data->request))); */
	/* print_service_options(service_data->options, 1 ); */
	init_rest_transfer((AIService *) ai_service);
	return RETURN_ZERO;
}


/*
 * Function to cleanup the transfer structures before initiating a new
 * transfer request.
 */
int
gpt_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *) ai_service);
	return RETURN_ZERO;
}


/*
 * Function to initialize the service buffers for data tranasfer.
 */
void
gpt_set_service_buffers(RestRequest * rest_request,
						RestResponse * rest_response,
						ServiceData * service_data)
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
int
gpt_add_service_headers(CURL * curl, struct curl_slist **headers,
						void *service)
{
	AIService  *ai_service = (AIService *) service;
	struct curl_slist *curl_headers = *headers;
	char		key_header[128];

	curl_headers = curl_slist_append(curl_headers,
									 "Content-Type: application/json");
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
#define GPT_MODEL_KEY			"model"
#define GPT_PROMPT_KEY 			"prompt"
#define GPT_MAX_TOKENS_KEY 		"max_tokens"
#define GPT_MAX_TOKENS_VAUE 	"1024"
#define JSON_DATA_TYPE_STRING	 "string"
#define JSON_DATA_TYPE_INTEGER	 "integer"

void
gpt_post_header_maker(char *buffer, const size_t maxlen,
					  const char *data, const size_t len)
{
	const char *data_types[] = {JSON_DATA_TYPE_STRING,
		JSON_DATA_TYPE_STRING,
	JSON_DATA_TYPE_INTEGER};
	const char *keys[] = {GPT_MODEL_KEY, GPT_PROMPT_KEY, GPT_MAX_TOKENS_KEY};
	const char *values[] = {MODEL_OPENAI_GPT_NAME, data, GPT_MAX_TOKENS_VAUE};

	generate_json(buffer, keys, values, data_types, 3);
}


/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 */
void
gpt_rest_transfer(void *service)
{
	Datum		choices;
	Datum		first_choice;
	Datum		return_text;
	AIService  *ai_service;

	ai_service = (AIService *) (service);
	rest_transfer(ai_service);
	*((char *) (ai_service->rest_response->data) + ai_service->rest_response->data_size) = '\0';

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
														  (char *) (ai_service->rest_response->data)),
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
		if (ai_service->service_data->response[i] == '\n')
			ai_service->service_data->response[i] = ' ';
		else
			break;
}
