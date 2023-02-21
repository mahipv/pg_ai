#ifndef _AI_CONFIG_H_
#define _AI_CONFIG_H_

#include <stdio.h>
#include <curl/curl.h>
#include "service_option.h"

/* Provider and the service names */
#define SERVICE_PROVIDER_OPEN_AI "OpenAI"
#define SERVICE_CHAT_GPT "chatgpt"
#define SERVICE_DALL_E2 "dall-e2"
#define UNSUPPORTED_SERVICE_MSG "Service is not supported."

/* buffer sizes */
#define SERVICE_MAX_REQUEST_SIZE (1 * 1024 * 1024)
#define SERVICE_MAX_RESPONSE_SIZE (1 * 1024 * 1024)

/* default array lengths used within pg_ai */
#define PG_AI_NAME_LENGTH 80

/* service data sizes */
#define SERVICE_DATA_SIZE (1 * 1024)

/* http responses */
#define HTTP_OK 200

/* internal error codes, for use within pg_ai */
#define RETURN_ZERO 0
#define RETURN_ERROR 1
#define RETURN_NETWORK_ERROR 2
#define RETURN_OOM 3
#define RETURN_OUT_OF_BOUNDS 4

/* limit to read the json into a buffer */
#define OPTIONS_FILE_SIZE 64*1024

/* help buffer size */
#define MAX_HELP_SIZE 4*1024

/* TODO this has be replaced by dynamic READ call back */
#define POST_DATA_SIZE 16*1024

#define ERROR_PREFIX "PG_AI Error"
#define TRANSFER_FAIL_MSG "Transfer call failed. Try again."
#define BIG_DATA_FAIL_MSG "Data to big, model only supports %lu words."

/* -----------------chatgpt---------- */
#define CHAT_GPT_API_URL "https://api.openai.com/v1/completions"
#define CHAT_GPT_SUMMARY_PROMPT "Get summary of the following in 1 lines."
#define CHAT_GPT_AGG_PROMPT "Get topic name for the words here."
#define CHAT_GPT_DESCRIPTION "Open AI's text-to-text model "

#define APPROX_WORDS_PER_1K_TOKENS 400
/* TODO to be dynamically picked with model from options */
#define CHAT_GPT_DAVINCI_MAX_TOKEN			(4 * 1024)
#define CHAT_GPT_DAVINCI_MAX_PROMPT_TOKEN	(3 * 1024) /* 75 */
#define CHAT_GPT_DAVINCI_MAX_RESULT_TOKEN	(1 * 1024) /* 25 */
#define CHAT_GPT_DAVINCI_MAX_PROMPT_WC      (CHAT_GPT_DAVINCI_MAX_PROMPT_TOKEN/1024 * \
											 APPROX_WORDS_PER_1K_TOKENS)

#define RESPONSE_JSON_CHOICE "choices"
#define RESPONSE_JSON_KEY "text"

#define CHAT_GPT_HELP "Usage:\n"\
	"get_insight(\"chatgpt\", <column name>, '<json options file>') \n" \
	"get_insight_agg(\"chatgpt\", <column name>, '<json options file>') \n"
/* -----------------chatgpt---------- */

/* -----------------dalle-2---------- */
#define DALLE_E2_API_URL "https://api.openai.com/v1/images/generations"
#define DALLE_E2_SUMMARY_PROMPT "Make a picture with the following"
#define DALLE_E2_AGG_PROMPT "Make a picture with the following "
#define DALLE_E2_DESCRIPTION "Open AI's text-to-image model "

#define RESPONSE_JSON_DATA "data"
#define RESPONSE_JSON_URL "url"

#define DALLE_E2_HELP "Usage:\n"\
	"get_insight(\"dalle-2\", <column name>, '<json options file>') \n"\
	"get_insight_agg(\"dalle-2\", <column name>, '<json options file>') \n"
/* -----------------dalle-2---------- */

/* -----------------8<--json options---------- */
#define OPTION_PROVIDERS "providers"
#define OPTION_PROVIDER_NAME "name"
#define OPTION_PROVIDER_KEY "key"

#define OPTION_PROVIDERS_DESC "The provider."
#define OPTION_PROVIDER_NAME_DESC "The AI service provider."
#define OPTION_PROVIDER_KEY_DESC "Key value to access the service (from service provider)."

#define OPTION_SERVICES "services"
#define OPTION_SERVICE_NAME "name"
#define OPTION_SERVICE_PROMPT "prompt"
#define OPTION_SERVICE_PROMPT_AGG "promptagg"

#define OPTION_SERVICES_DESC "The AIservices offered by the provider."
#define OPTION_SERVICE_NAME_DESC "The name of the AI service."
#define OPTION_SERVICE_PROMPT_DESC "The prompt used for the get_insight() function."
#define OPTION_SERVICE_PROMPT_AGG_DESC "The prompt used for the get_insight_agg() function."
/* ---------------------json options-->8------ */

#endif /* _AI_CONFIG_H_*/
