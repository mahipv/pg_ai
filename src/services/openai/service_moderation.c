#include "service_moderation.h"

#include "rest/rest_transfer.h"
#include "core/utils_pg_ai.h"

/*
 * Function to define aptions applicable to the moderation service calls.
 * These options are to be passed via SQL function calls or set by GUCs.
 */
static void define_options(AIService *ai_service)
{
	ServiceOption **option_list = &(ai_service->service_data->options);

	/* define the common options from the base */
	ai_service->define_common_options(ai_service);

	define_new_option(option_list, OPTION_COLUMN_VALUE,
					  OPTION_COLUMN_VALUE_DESC, OPTION_FLAG_HELP_DISPLAY,
					  ai_service->service_data->request,
					  SERVICE_MAX_REQUEST_SIZE);
}

/*
 * Initialize the options to be used for moderations. The options will hold
 * information about the AI service and some of them will be used in the curl
 * headers for REST transfer.
 */
void moderation_initialize_service(void *service)
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
 * Return the help text to be displayed for the moderation service.
 */
void moderation_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, MODERATION_HELP, max_len);
}

/*
 * Function to set the options for the gpt service. The options are set from
 * the function arguments or from the GUCs. The options are validated and
 * the function returns an error if any of the required options are not set.
 */
int moderation_set_and_validate_options(void *service, void *function_options)
{
	AIService *ai_service = (AIService *)service;
	ServiceOption *options = ai_service->service_data->options;

	/* check if all required args are set */
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
 * This function is called from the PG layer after it has the table data and
 * before invoking the REST transfer call. Load the json options(will be used
 * for the transfer)and copy the data received from PG into the REST request
 * structures.
 */
int moderation_set_service_data(void *service, void *data)
{
	AIService *ai_service = (AIService *)service;
	bool concat = false;

	/* if aggregate function then concat */
	if (ai_service->function_flags & FUNCTION_MODERATION_AGGREGATE)
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
int moderation_prepare_for_transfer(void *service)
{
	AIService *ai_service = (AIService *)service;
	ServiceOption *option_list = ai_service->service_data->options;
	char prompt[2];
	size_t prompt_len;
	ServiceOption *option;

	strcpy(prompt, "\"");

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
int moderation_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *)ai_service);
	return RETURN_ZERO;
}

/*
 * Function to initialize the service buffers for data tranasfer.
 */
void moderation_set_service_buffers(RestRequest *rest_request,
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
int moderation_add_service_headers(CURL *curl, struct curl_slist **headers,
								   void *service)
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
 */
#define MODERATION_PREFIX "{\"input\":\""
#define MODERATION_POST_PREFIX "\"}"
void moderation_post_header_maker(char *buffer, const size_t maxlen,
								  const char *data, const size_t len)
{
	strcpy(buffer, MODERATION_PREFIX);
	strcat(buffer, data);
	strcat(buffer, MODERATION_POST_PREFIX);
	/* ereport(INFO, (errmsg("Post header: %s\n", buffer))); */
}

/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 */
void moderation_rest_transfer(void *service)
{
	AIService *ai_service;

	ai_service = (AIService *)(service);
	rest_transfer(ai_service);

	/* truncate the response */
	*((char *)(ai_service->rest_response->data) +
	  ai_service->rest_response->data_size) = '\0';

	if (ai_service->rest_response->response_code == HTTP_OK)
	{
		strcpy(ai_service->service_data->response,
			   (char *)(ai_service->rest_response->data));
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
