#include "rest_transfer.h"
#include "postgres.h"
#include "pg_ai_utils.h"

/*
 * Initialize the transfer buffers required for the REST transfer.
 * TODO these pointers to be cached to be reused between calls.
 *
 * @param[in/out]	ai_service		the AIService currently in use
 * @return			void
 *
 */
void
init_rest_transfer(AIService *ai_service)
{
	if (!ai_service->rest_request)
		ai_service->rest_request = (RestRequest*)palloc(sizeof(RestRequest));
	ai_service->rest_request->data_size = 0;

	if (!ai_service->rest_response)
		ai_service->rest_response = (RestResponse*)palloc(sizeof(RestResponse));
	ai_service->rest_response->data_size = 0;

	(ai_service->set_service_buffers)(ai_service->rest_request,
									  ai_service->rest_response,
									  ai_service->service_data);
	ai_service->rest_request->data_size = strlen(ai_service->rest_request->data);

}

/*
 * Cleanup the buffers used for the REST transfer
 * TODO these pointers to be cached to be reused between calls.
 *
 * @param[in/out]	ai_service		the AIService currently in use
 * @return			void
 *
 */
void
cleanup_rest_transfer(AIService *ai_service)
{
	if (ai_service->rest_request)
		free(ai_service->rest_request);
	ai_service->rest_request = NULL;

	if (ai_service->rest_response)
		free(ai_service->rest_response);
	ai_service->rest_response = NULL;
}

/*
 * The callback function called by curl library when it receives data.
 * Save the data received for further processing. This callback might
 * get called more than once while receiving the response. The data
 * received by this function is not null terminated.
 *
 * @param[in]	contents	the data returned from the service
 * @param[in]	size		is always 1
 * @param[in]	nmemb		the size of the data being returned
 * @param[in]	userp		user data pointer that was passed to curl
 * @return		size_t		the exact number of bytes processed by
 *							this callback. If not same as nmemb then
 *							the transfer is aborted.
 *
 */
static size_t
write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	RestResponse *response = (RestResponse *)userp;

	if (response->data_size + realsize > response->max_size)
	{
		fprintf(stderr, "not enough memory (realloc returned NULL)\n");
        return 0;
	}
	fprintf(stderr, "real size %ld\n", realsize);
    memcpy((char*)(response->data)+response->data_size, (char *)contents, realsize);
    response->data_size += realsize;
    return realsize;
}

/*
 * The callback function called by curl library when it has to send data.
 * TODO enable this to pass on large data
 *
 * @param[out]	contents	data should be filled in this buffer
 * @param[in]	size		is always 1
 * @param[in]	nmemb		max size the buffer can take
 * @param[in]	userp		user data pointer that was passed to curl
 * @return		size_t		the exact number of bytes filled in the
 *							buffer. returning 0 will be treated as
 *							end of transfer.
 *
 */
static size_t
read_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	return 0;
}

/*
 * The callback function called by curl library with debug information if
 * CURLOPT_VERBOSE is set. The data will not be null terminated.
 *
 * @param[in]	curl		the curl handle
 * @param[in]	type		the meta data (curl_infotype enum)
 * @param[in]	data		the debug data
 * @param[in]	size		size of the debug data
 * @param[in]	userp		user data pointer that was passed to curl
 * @return		int			this callback is expected to return 0
 *
 */
static int
debug_curl(CURL *curl, curl_infotype type, char *data, size_t size, void *userp)
{
	size_t 			i;
	size_t 			c;
	unsigned int 	display_width=0x10;
	const char 		*delimiter_start = "\n----8<----\n";
	const char 		*delimiter_end = "\n---->8----\n";

	fprintf(stderr, "%s, %10.10ld bytes (0x%8.8lx)\n",
			delimiter_start, (long)size, (long)size);

	for(i=0; i<size; i+= display_width)
	{
		fprintf(stderr, "%4.4lx: ", (long)i);
		/* hex on the left */
		for(c = 0; c < display_width; c++)
		{
			if (i+c < size)
				fprintf(stderr, "%02x ", data[i+c]);
			else
				fputs("   ", stderr);
		}
		/* ascii on the right */
		for(c = 0; (c < display_width) && (i+c < size); c++)
		{
			char x = (data[i+c] >= 0x20 && data[i+c] < 0x80) ? data[i+c] : '.';
			fputc(x, stderr);
		}
		fputc('\n', stderr); /* newline */
	}

	fprintf(stderr, "%s, %10.10ld bytes (0x%8.8lx)\n", delimiter_end, (long)size, (long)size);
	return 0;
}

/*
 * Helper function to make the REST headers. Makes call to the service specific
 * callback to make the headers.
 *
 * @param[in/out]	the curl handle to which headers are to be added.
 * @param[in]		ai_service	pointer to the AIService
 * @return			void
 *
 */
static void
make_curl_headers(CURL *curl, AIService *ai_service)
{
	struct curl_slist *headers = NULL;
	curl_easy_setopt(curl, CURLOPT_URL, ai_service->service_data->url);

	// TODO Assert
	if (ai_service->add_service_headers)
		(ai_service->add_service_headers)(curl, &headers, ai_service);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
}

/*
 * Helper function to check the word count to be passed to the service.
 * TODO this has to get the word cound dynamically from the model used
 *
 * @param[in]	text			the string that is to be word counted
 * @param[in]	max_supported	the maximum no of words supported by the
								service per call.
 * @return		int				zero if valid, non-zero otherwise
 *
 */
static int
vaildate_data_size(const char *text, size_t *max_supported)
{
	*max_supported = CHAT_GPT_DAVINCI_MAX_PROMPT_WC;
	return get_word_count(text, *max_supported, NULL);
}

/*
 * The function to make the final REST transfer using curl.
 * TODO get the headers/data request & response with the service callbacks
 *
 * @param[in]	ai_service	pointer to the AIService which for which transfer
 *							is to be made.
 * @return		void
 *
 */
void
rest_transfer(AIService *ai_service)
{

	CURL 		*curl;
	CURLcode 	res;
	char 		post_data[POST_DATA_SIZE];
	char 		*encoded_prompt;
	char		error_msg[1024];
	size_t		max_word_count;

	/* TODO check for the size dynamically even before the trasfer is called */
	if (vaildate_data_size(ai_service->rest_request->data, &max_word_count))
	{
		ai_service->rest_response->response_code = 0x2;
		sprintf(error_msg, BIG_DATA_FAIL_MSG, max_word_count);
		strcpy(ai_service->rest_response->data, error_msg);
		ai_service->rest_response->data_size = strlen(error_msg);
		return;
	}

	curl = curl_easy_init();
	if (curl)
    {
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_curl);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		make_curl_headers(curl, ai_service);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(ai_service->rest_response));

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, (void *)(ai_service->rest_request));

		/* TODO POST DATA has to be moved to respective service */
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		encoded_prompt = curl_easy_escape(curl, ai_service->rest_request->data,
										  ai_service->rest_request->data_size);
		(ai_service->post_header_maker)(post_data, POST_DATA_SIZE,
										encoded_prompt, sizeof(encoded_prompt));
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
		{
			ai_service->rest_response->response_code = 0x1;
			strcpy(ai_service->rest_response->data, TRANSFER_FAIL_MSG);
			ai_service->rest_response->data_size = strlen(TRANSFER_FAIL_MSG);
		}
		else
		{
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
							  &ai_service->rest_response->response_code);
		}
		curl_easy_cleanup(curl);
    }
}
