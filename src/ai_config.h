#ifndef _AI_CONFIG_H_
#define _AI_CONFIG_H_

#include <stdio.h>

//#define OPEN_AI_API_KEY ""

/* supported ai service names */
#define SERVICE_CHAT_GPT "chatgpt"
#define SERVICE_DALLE_2 "dalle-2"

#define OPEN_AI_KEY ""
/*---8<---- to be parameterized -------------*/

#define UNSUPPORTED_SERVICE_MSG "Service is not supported."

/* buffer sizes */
#define SERVICE_MAX_REQUEST_SIZE 1 * 1024 * 1024
#define SERVICE_MAX_RESPONSE_SIZE 1 * 1024 * 1024

/* commonly used for character arrays */
#define PG_AI_LINE_LENGTH 80

/* service data sizes */
#define SERVICE_DATA_SIZE 1 * 1024

/* http responses */
#define HTTP_OK 200

typedef struct RestRequestData
{
	void		*headers;
	void		*data;
	size_t  	data_size;
	size_t  	max_size;
	void		*prompt;
}RestRequest;


typedef struct RestResponseData
{
	long     	response_code;
	void		*headers;
	void		*data;
	size_t  	data_size;
	size_t  	max_size;
}RestResponse;


typedef struct ServiceData
{
	char	url[SERVICE_DATA_SIZE];
	char    key[SERVICE_DATA_SIZE];
	char 	request[SERVICE_DATA_SIZE];
	size_t  max_request_size;
	char 	prompt[SERVICE_DATA_SIZE];
	size_t  max_prompt_size;
	char    response[SERVICE_DATA_SIZE];
	size_t  max_response_size;
}ServiceData;

#endif /* _AI_CONFIG_H_*/
