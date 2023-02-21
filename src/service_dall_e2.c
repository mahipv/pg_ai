#include "service_dall_e2.h"
#include <ctype.h>
#include <malloc.h>
#include "postgres.h"
#include "utils/builtins.h"
#include "pg_ai_utils.h"
#include "rest_transfer.h"

/*
 * Method to define aptions applicable to dalle2 service. The values for this
 * options are read from the json options file.
 *
 * Parameters:
 * service_data	[out]	options added to the service specific data
 *
 * Return:		None
 *
 */
static void
define_options(ServiceData  *service_data)
{
	define_new_option(&(service_data->options), OPTION_PROVIDER_KEY,
					  OPTION_PROVIDER_KEY_DESC, 1/*provider*/, 1/*required*/);
	define_new_option(&(service_data->options), OPTION_SERVICE_PROMPT,
					  OPTION_SERVICE_PROMPT_DESC ,0/*provider*/, 0/*required*/);
	define_new_option(&(service_data->options), OPTION_SERVICE_PROMPT_AGG,
				   OPTION_SERVICE_PROMPT_AGG_DESC, 0/*provider*/,  0/*required*/);
}

/*
 * Initialize the options to be used for dalle2. The options will hold
 * information about the AI service and some of them will be used in the curl
 * headers for REST transfer.
 *
 * @param[out]	service_data	options added to the service specific data
 * @return		void
 *
 */
void
dalle_e2_init_service_options(void *service)
{
	AIService 		*ai_service = (AIService *)service;
	ServiceData		*service_data;

	service_data = (ServiceData*)palloc0(sizeof(ServiceData));
	ai_service->service_data = service_data;
	strcpy(ai_service->service_data->provider, SERVICE_PROVIDER_OPEN_AI);
	strcpy(ai_service->service_data->name, SERVICE_DALL_E2);
	strcpy(ai_service->service_data->description, DALLE_E2_DESCRIPTION);
	define_options(ai_service->service_data);
}

/*
 * Return the help text to be displayed for the dalle2 service
 *
 * @param[out]	help_text	the help text is reurned in this array
 * @param[in]	max_len		the max length of the help text the array can hold
 * @return		void
 *
 */
void
dalle_e2_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, DALLE_E2_HELP, max_len);
}

/*
 * Read the json options file and set the values in the options list for
 * the dalle2 service
 *
 * @param[in]		file_path	the help text is reurned in this array
 * @param[in/out]	ai_service	pointer to the AIService which has the
 *								defined option list
 * @return			zero on success, non-zero otherwise
 *
 */
int
dalle_e2_add_service_headers(CURL *curl, struct curl_slist **headers, void *service)
{
	AIService 			*ai_service = (AIService*)service;
	struct curl_slist 	*curl_headers = *headers;
	char 				key_header[128];

	curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
	snprintf(key_header, sizeof(key_header), "Authorization: Bearer %s",
			get_option_value(ai_service->service_data->options, OPTION_PROVIDER_KEY));
	curl_headers = curl_slist_append(curl_headers, key_header);
	*headers = curl_headers;

	return RETURN_ZERO;
}

/*
 * Read the json options file and set the values in the options list for
 * the dalle2 service
 *
 * @param[in]		file_path	the help text is reurned in this array
 * @param[in/out]	ai_service	pointer to the AIService which has the
 *								defined option list
 * @return			zero on success, non-zero otherwise
 *
 */
static int
load_and_validate_options(const char *file_path, AIService *ai_service)
{
	ServiceOption 	*options = ai_service->service_data->options;
	int 			ret_val = 0;
	while (options)
	{
		ret_val = get_option_from_file(file_path,
									   (ai_service->get_provider_name)(ai_service),
									   (ai_service->get_service_name)(ai_service),
									   options->provider,
									   options->name, options->value, OPTION_VALUE_LEN);

		/* unable to read the option value */
		if (ret_val)
		{
			/* required and not set */
			if (options->required)
			{
				ereport(INFO,(errmsg("Missing value for %s .\n",options->name)));
				return RETURN_ERROR;
			}
			else
			{
				if (!strcmp(options->name, OPTION_SERVICE_PROMPT))
					strcpy(options->value, DALLE_E2_AGG_PROMPT);

				if (!strcmp(options->name, OPTION_SERVICE_PROMPT_AGG))
					strcpy(options->value, DALLE_E2_SUMMARY_PROMPT);
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
dalle_e2_init_service_data(void *options, void *service, void *file_path)
{
	ServiceData		*service_data;
	char 			*column_data;
	AIService		*ai_service = (AIService *)service;

	service_data = ((AIService*)ai_service)->service_data;
	column_data = options;

	service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	service_data->max_response_size = SERVICE_MAX_REQUEST_SIZE;

	/* TODO convert all these to the options to be read from the option file */
	/* initialize data partly here */
	strcpy(service_data->url, DALLE_E2_API_URL);

	/* initialize read the rest of the data from file */
	if (load_and_validate_options((char*)file_path, ai_service))
		return RETURN_ERROR;

	if (ai_service->is_aggregate)
		strcpy(service_data->request,
			   get_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT_AGG));
	else
		strcpy(service_data->request,
			   get_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT));

	strcat(service_data->request, " \"");
	strcat(service_data->request, column_data);
	strcat(service_data->request, "\"");

	//print_service_options(service_data->options, 1 /*print values*/);
	init_rest_transfer((AIService *)ai_service);
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
dalle_e2_cleanup_service_data(void *ai_service)
{
	cleanup_rest_transfer((AIService *)ai_service);
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
dalle_e2_set_service_buffers(RestRequest *rest_request,
						   RestResponse *rest_response,
						   ServiceData *service_data)
{
	rest_request->data = service_data->request;
	rest_request->max_size = service_data->max_request_size;

	rest_response->data = service_data->response;
	rest_response->max_size = service_data->max_response_size;
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
dalle_e2_rest_transfer(void *service)
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
#define DALLE_E2_PRE_PREFIX "{\"prompt\":\""
#define DALLE_E2_POST_PREFIX "\",\"num_images\":1,\"size\":\"1024x1024\",\"response_format\":\"url\"}"
void
dalle_e2_post_header_maker(char *buffer, const size_t maxlen,
						 const char *data, const size_t len)
{
	strcpy(buffer, DALLE_E2_PRE_PREFIX);
	strcat(buffer, data);
	strcat(buffer, DALLE_E2_POST_PREFIX);
}
