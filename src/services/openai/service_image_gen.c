#include "service_image_gen.h"

#include "utils/builtins.h"

#include "utils_pg_ai.h"
#include "rest/rest_transfer.h"

/*
* Function to define aptions applicable to the image gen service calls.
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
	if (ai_service->function_flags & FUNCTION_GENERATE_IMAGE)
		define_new_option(option_list, OPTION_SERVICE_PROMPT,
					  	  OPTION_SERVICE_PROMPT_DESC, false /* guc */ ,
					  	  true /* required */ , true /* help_display */ );	
	
	/* options for the aggregate function */
	if (ai_service->function_flags & FUNCTION_GENERATE_IMAGE_AGGREGATE)
		define_new_option(option_list, OPTION_SERVICE_PROMPT_AGG,
						  OPTION_SERVICE_PROMPT_AGG_DESC, false /* guc */ ,
						  true /* required */ , true /* help_display */ );
}

/*
 * Initialize the options to be used for dall-e. The options will hold
 * information about the AI service and some of them will be used in the curl
 * headers for REST transfer.
 */
void
image_gen_init_service_options(void *service)
{
	AIService  *ai_service = (AIService *) service;
	ServiceData *service_data;

	service_data = (ServiceData *) palloc0(sizeof(ServiceData));
	ai_service->service_data = service_data;
	/* Define the options for this service */
	define_options(ai_service);
}


/*
 * Return the help text to be displayed for the dalle2 service
 */
void
image_gen_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, IMAGE_GEN_HELP, max_len);
}


/*
* Function to set the options for the image_gen service. The options
* are set from function arguments or from the GUCs. The options are
* validated andthe function returns an error if any of the required
* options are not set.
*/
int
image_gen_set_and_validate_options(void *service, void *function_options)
{
	PG_FUNCTION_ARGS = (FunctionCallInfo) function_options;
	AIService  *ai_service = (AIService *) service;
	ServiceOption *options = ai_service->service_data->options;
	int			arg_offset;

	/* aggregate functions get an extra argument at position 0 */
	arg_offset = (ai_service->function_flags &
				  FUNCTION_GENERATE_IMAGE_AGGREGATE) ? 1 : 0;

	if ((ai_service->function_flags & FUNCTION_GENERATE_IMAGE) &&
		(!PG_ARGISNULL(0 + arg_offset)))
		set_option_value(options, OPTION_COLUMN_VALUE,
						 text_to_cstring(PG_GETARG_TEXT_P(0 + arg_offset)),
						 NULL /* value_ptr */ , 0 /* size */ );

	/* set the default prompt or passed prompt based on the function */
	if (!PG_ARGISNULL(1 + arg_offset))
	{
		if (ai_service->function_flags & FUNCTION_GENERATE_IMAGE_AGGREGATE)
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
		if (ai_service->function_flags & FUNCTION_GENERATE_IMAGE_AGGREGATE)
			set_option_value(options, OPTION_SERVICE_PROMPT_AGG,
							 IMAGE_GEN_AGG_PROMPT, NULL /* value_ptr */ ,
							 0 /* size */ );
		else
			set_option_value(options, OPTION_SERVICE_PROMPT, IMAGE_GEN_PROMPT,
							 NULL /* value_ptr */ , 0 /* size */ );
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
image_gen_init_service_data(void *options, void *service, void *data)
{
	ServiceData *service_data;
	char	   *column_data;
	AIService  *ai_service = (AIService *) service;

	service_data = ((AIService *) ai_service)->service_data;

	if (ai_service->function_flags & FUNCTION_GENERATE_IMAGE)
		column_data = get_option_value(ai_service->service_data->options, OPTION_COLUMN_VALUE);
	else
		column_data = (char *) data;

	service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	service_data->max_response_size = SERVICE_MAX_REQUEST_SIZE;

	/* print_service_options(service_data->options, 1); */

	if (ai_service->function_flags & FUNCTION_GENERATE_IMAGE)
		strcpy(service_data->request,
			   get_option_value(ai_service->service_data->options,
								OPTION_SERVICE_PROMPT));

	if (ai_service->function_flags & FUNCTION_GENERATE_IMAGE_AGGREGATE)
		strcpy(service_data->request,
			   get_option_value(ai_service->service_data->options,
								OPTION_SERVICE_PROMPT_AGG));

	strcat(service_data->request, " \"");
	strcat(service_data->request, column_data);
	strcat(service_data->request, "\"");

	init_rest_transfer((AIService *) ai_service);
	return RETURN_ZERO;
}


/*
 * Function to cleanup the transfer structures before initiating a new
 * transfer request.
 */
int
image_gen_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *) ai_service);
	return RETURN_ZERO;
}

/*
 * Function to initialize the service buffers for data tranasfer.
 */
void
image_gen_set_service_buffers(RestRequest * rest_request,
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
image_gen_add_service_headers(CURL * curl, struct curl_slist **headers,
							  void *service)
{
	AIService  *ai_service = (AIService *) service;
	struct curl_slist *curl_headers = *headers;
	char		key_header[128];

	curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
	snprintf(key_header, sizeof(key_header), "Authorization: Bearer %s",
			 get_option_value(ai_service->service_data->options,
							  OPTION_SERVICE_API_KEY));
	curl_headers = curl_slist_append(curl_headers, key_header);
	*headers = curl_headers;

	return RETURN_ZERO;
}


/* TODO */
/*
 * Callback to make the post header
 */
#define IMAGE_GEN_PRE_PREFIX "{\"prompt\":\""
#define IMAGE_GEN_POST_PREFIX "\",\"num_images\":1,\"size\":\"1024x1024\",\"response_format\":\"url\"}"
void
image_gen_post_header_maker(char *buffer, const size_t maxlen,
							const char *data, const size_t len)
{
	strcpy(buffer, IMAGE_GEN_PRE_PREFIX);
	strcat(buffer, data);
	strcat(buffer, IMAGE_GEN_POST_PREFIX);
	/* ereport(INFO, (errmsg("Post header: %s\n", buffer))); */
}


/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 */
void
image_gen_rest_transfer(void *service)
{
	Datum		data;
	Datum		url;
	Datum		return_text;
	AIService  *ai_service;

	ai_service = (AIService *) (service);
	rest_transfer(ai_service);
	*((char *) (ai_service->rest_response->data) + ai_service->rest_response->data_size) = '\0';

	if (ai_service->rest_response->response_code == HTTP_OK)
	{
		data = DirectFunctionCall2(json_object_field_text,
								   CStringGetTextDatum(
													   (char *) (ai_service->rest_response->data)),
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
		strcpy(ai_service->service_data->response,
			   "Something is not ok, try again.");
	}

	/* remove prefexing \n */
	for (int i = 0; ai_service->service_data->response[i] != '\0'; i++)
		if (ai_service->service_data->response[i] != '\n')
			break;
		else
			ai_service->service_data->response[i] = ' ';
}
