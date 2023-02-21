#include "rest_transfer.h"
#include "postgres.h"

#define TRANSFER_FAIL_MSG "Transfer call failed. Try again."

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

static size_t
read_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	return 0;
}

static int
debug_curl(CURL *curl, curl_infotype type, char *data, size_t size, void *userp)
{
	size_t 			i;
	size_t 			c;
	unsigned int 	display_width=0x10;
	const char 		*delimiter_start = "\n----8<----\n";
	const char 		*delimiter_end = "\n---->8----\n";

	fprintf(stderr, "%s, %10.10ld bytes (0x%8.8lx)\n", delimiter_start, (long)size, (long)size);

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

static void
make_curl_headers(CURL *curl, AIService *ai_service)
{
	struct curl_slist *headers = NULL;
	char key_header[128];

	curl_easy_setopt(curl, CURLOPT_URL, ai_service->service_data->url);

	headers = curl_slist_append(headers, "Content-Type: application/json");
	snprintf(key_header, sizeof(key_header), "Authorization: Bearer %s",
			 ai_service->service_data->key);
	headers = curl_slist_append(headers, key_header);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

}

#define POST_DATA_SIZE 16*1024
void
rest_transfer(AIService *ai_service)
{
	CURL 		*curl;
    CURLcode 	res;
	char 		post_data[POST_DATA_SIZE];
	char 		*encoded_prompt;

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


