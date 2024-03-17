#include "service_gpt.h"
#include <ctype.h>
#include <malloc.h>
#include "utils/builtins.h"
#include "postgres.h"
#include "ai_config.h"
#include "utils_pg_ai.h"
#include "rest/rest_transfer.h"
#include "guc/pg_ai_guc.h"
/*
 * Function to define aptions applicable to chatgpt service. The values for
 * this options are read from the json options file.
 *
 * @param[out]	service_data	options added to the service specific data
 * @return		void
 *
 */
static void
define_options(AIService * ai_service)
{
	ServiceData *service_data = ai_service->service_data;

	define_new_option(&(service_data->options), OPTION_SERVICE_NAME,
					  OPTION_SERVICE_NAME_DESC, 1 /* guc */ , 1 /* required */ , false /* help_display */ );
	define_new_option(&(service_data->options), OPTION_MODEL_NAME,
					  OPTION_MODEL_NAME_DESC, 1 /* guc */ , 1 /* required */ , false /* help_display */ );
	define_new_option(&(service_data->options), OPTION_INSIGHT_COLUMN,
					  OPTION_INSIGHT_COLUMN_DESC, 0 /* guc */ , 0 /* required */ , true /* help_display */ );
	define_new_option(&(service_data->options), OPTION_SERVICE_API_KEY,
					  OPTION_SERVICE_API_KEY_DESC, 1 /* guc */ , 1 /* required */ , false /* help_display */ );
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT)
		define_new_option(&(service_data->options), OPTION_SERVICE_PROMPT,
						  OPTION_SERVICE_PROMPT_DESC, 0 /* guc */ , 1 /* required */ , true /* help_display */ );
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
		define_new_option(&(service_data->options), OPTION_SERVICE_PROMPT_AGG,
						  OPTION_SERVICE_PROMPT_AGG_DESC, 0 /* guc */ , 1 /* required */ , true /* help_display */ );
}

/*
 * Initialize the options to be used for chatgpt. The options will hold
 * information about the AI service and some of them will be used in the curl
 * headers for REST transfer.
 *
 * @param[out]	service_data	options added to the service specific data
 * @return		void
 *
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
 * Return the help text to be displayed for the chatgpt service
 *
 * @param[out]	help_text	the help text is reurned in this array
 * @param[in]	max_len		the max length of the help text the array can hold
 * @return		void
 *
 */
void
gpt_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, GPT_HELP, max_len);
}

int
gpt_set_and_validate_options(void *service, void *function_options)
{
	PG_FUNCTION_ARGS = (FunctionCallInfo) function_options;
	AIService  *ai_service = (AIService *) service;
	ServiceOption *options;
	int			arg_offset;

	/* aggregate functions get an extra argument at position 0 */
	arg_offset = (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE) ? 1 : 0;

	if (!PG_ARGISNULL(0 + arg_offset))
		set_option_value(ai_service->service_data->options, OPTION_INSIGHT_COLUMN, text_to_cstring(PG_GETARG_TEXT_P(0 + arg_offset)));

	if (!PG_ARGISNULL(1 + arg_offset))
	{
		if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
			set_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT_AGG, text_to_cstring(PG_GETARG_TEXT_P(1 + arg_offset)));
		else
			set_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT, text_to_cstring(PG_GETARG_TEXT_P(1 + arg_offset)));
	}
	else						/* set the default prompts */
	{
		if (ai_service->function_flags & FUNCTION_GET_INSIGHT_AGGREGATE)
			set_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT_AGG, GPT_AGG_PROMPT);
		else
			set_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT, GPT_SUMMARY_PROMPT);
	}

	/* check if all required args are set */
	options = ai_service->service_data->options;
	while (options)
	{
		if (options->is_set)
		{
			options = options->next;
			continue;
		}
		else
		{
			/* required and not set */
			if (options->required)
			{
				ereport(INFO, (errmsg("Required %s option \"%s\" missing.\n",
									  options->guc_option ? "GUC" : "function", options->name)));
				return RETURN_ERROR;
			}
		}
		options = options->next;
	}
	return RETURN_ZERO;
}

/*
 * This function is called from the PG layer after it has the table data and
 * before invoking the REST transfer call. Load the json options(will be used
 * for the transfer)and copy the data received from PG into the REST request
 * structures.
 *
 * @param[in]		file_path	the help text is reurned in this array
 * @param[in/out]	ai_service	pointer to the AIService which has the
 *								defined option list
 * @return			zero on success, non-zero otherwise
 *
 */
int
gpt_init_service_data(void *options, void *service, void *data)
{
	ServiceData *service_data;
	char	   *column_data;
	AIService  *ai_service = (AIService *) service;

	service_data = ai_service->service_data;
	if (ai_service->function_flags & FUNCTION_GET_INSIGHT)
		column_data = get_option_value(ai_service->service_data->options, OPTION_INSIGHT_COLUMN);
	else
		column_data = (char *) data;

	service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	service_data->max_response_size = SERVICE_MAX_RESPONSE_SIZE;

	/* initialize data partly here */
	strcpy(service_data->url, GPT_API_URL);

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
 *
 * @param[in/out]	ai_service	pointer to the AIService which has the
 * @return			void
 *
 */
int
gpt_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *) ai_service);
	return RETURN_ZERO;
}

/*
 * Function to initialize the service buffers for data tranasfer.
 *
 * @param[out]	rest_request	pointer to the REST request data
 * @param[out]	rest_reponse	pointer to the REST response data
 * @param[in]	service_data	pointer to the service specific data
 * @return		void
 *
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
 *
 * @param[out]	curl			pointer curl conext
 * @param[out]	curl_slist		add new headers to this list
 * @param[in]	service_data	pointer to AIService
 * @return		zero on success, non-zero otherwise
 *
 */
int
gpt_add_service_headers(CURL * curl, struct curl_slist **headers,
						void *service)
{
	AIService  *ai_service = (AIService *) service;
	struct curl_slist *curl_headers = *headers;
	char		key_header[128];

	curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
	snprintf(key_header, sizeof(key_header), "Authorization: Bearer %s",
			 get_option_value(ai_service->service_data->options, OPTION_SERVICE_API_KEY));
	curl_headers = curl_slist_append(curl_headers, key_header);
	*headers = curl_headers;

	return RETURN_ZERO;
}

/* TODO the token count should be dynamic */
/*
 * Callback to make the post header
 *
 * @param[out]	buffer	the post header is returned in this
 * @param[in]	maxlen	the max length the buffer can accomodate
 * @param[in]	data	the data from which post header is created
 * @param[in]	len		length of the data
 * @return		void
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
	const char *data_types[] = {JSON_DATA_TYPE_STRING, JSON_DATA_TYPE_STRING, JSON_DATA_TYPE_INTEGER};
	const char *keys[] = {GPT_MODEL_KEY, GPT_PROMPT_KEY, GPT_MAX_TOKENS_KEY};
	const char *values[] = {MODEL_OPENAI_GPT_NAME, data, GPT_MAX_TOKENS_VAUE};

	generate_json(buffer, keys, values, data_types, 3);
}

/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 *
 * @param[in]	service		pointer to the service specific data
 * @return		void
 *
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
