#ifndef _AI_CONFIG_H_
#define _AI_CONFIG_H_

#include <stdio.h>
#include <curl/curl.h>
#include "service_option.h"

/* Provider and the service names */
#define SERVICE_PROVIDER_OPEN_AI "OpenAI"
#define SERVICE_CHAT_GPT "chatgpt"
#define SERVICE_DALL_E2 "dall-e2"
#define SERVICE_ADA "ada"
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

#define COLUMN_NAME_LEN 255

/* every service function has a unique constant */
#define CHAT_GPT_FUNCTION_GET_INSIGHT              0x00000001
#define CHAT_GPT_FUNCTION_GET_INSIGHT_AGGREGATE    0x00000002
#define DALLE2_GPT_FUNCTION_GET_INSIGHT            0x00000004
#define DALLE2_GPT_FUNCTION_GET_INSIGHT_AGGREGATE  0x00000008
#define ADA_FUNCTION_CREATE_VECTOR_STORE           0x00000010
#define ADA_FUNCTION_QUERY_VECTOR_STORE            0x00000020


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

#define CHAT_GPT_HELP "\nFunctions:\n"\
"(i) get_insight(service => 'chatgpt',\n\
                 key => '<OpenAI auth key>',\n\
                 textcolumn => <column name>,\n\
                 prompt => '<prompt> eg: Get summary of the following in 1 line')  \n\n" \
"(ii) get_insight_agg(\"chatgpt\",\n\
                      '<OpenAI auth key>',\n\
                      <column name>,\n\
                      '<prompt> eg: Choose a topic for the words here:') \n"
/* -----------------chatgpt---------- */

/* -----------------dalle-2---------- */
#define DALLE_E2_API_URL "https://api.openai.com/v1/images/generations"
#define DALLE_E2_SUMMARY_PROMPT "Make a picture with the following"
#define DALLE_E2_AGG_PROMPT "Make a picture with the following "
#define DALLE_E2_DESCRIPTION "Open AI's text-to-image model "

#define RESPONSE_JSON_DATA "data"
#define RESPONSE_JSON_URL "url"

#define DALLE_E2_HELP "\nFunctions:\n"\
"(i)  get_insight(service => 'dall-e2',\n\
                  key => '<OpenAI auth key>',\n\
                  textcolumn => <column name>,\n\
                  prompt => '<prompt> eg:Make a picture of the following:')\n\n" \
"(ii) get_insight_agg(\"dall-e2\",\n\
                      '<OpenAI auth key>',\n\
                      <column name>,\n\
                      '<prompt> eg: Make a picture with the following:') \n"
/* -----------------dalle-2---------- */


/* -----------------ada---------- */
#define ADA_API_URL "https://api.openai.com/v1/embeddings"
#define ADA_DESCRIPTION "Open AI's embeddings model(vectors)"

#define ADA_HELP "\nFunctions:\n"\
"(ii) create_vector_store(service => 'ada', \n\
                          key => '<OpenAI auth key>', \n\
                          store => '<new store name>', \n\
                          query => 'SQL query from which the store is made.', \n\
                          prompt => '<natural language prompt>' )\n\n" \
"(ii) query_vector_store(service => 'ada', \n\
                         key => '<OpenAI auth key>', \n\
                         store => '<new store name>', \n\
                         prompt => '<natural language prompt>', \n\
                         count => <count of records to fetch>, \n\
                         similarity_algorithm => '<vector matching algorithm>(default:cosine_similarity)')\n"

#define SQL_QUERY_MAX_LENGTH 256*1024
// seems const - TODO
#define ADA_EMBEDDINGS_LIST_SIZE 1536

#define EMBEDDINGS_COLUMN_NAME "embeddings"
#define PK_SUFFIX	"_id"
#define EMBEDDINGS_COSINE_SIMILARITY "cosine_similarity"

#define PG_EXTENSION_PG_VECTOR "vector"

#define MIN_COUNT_RECORDS 1
#define MAX_COUNT_RECORDS 10
/* -----------------ada---------- */

/* -----------------8<--Function Arguments ---------- */
#define OPTION_PROVIDERS "providers"
#define OPTION_PROVIDERS_DESC "The provider."

#define OPTION_PROVIDER_NAME "name"
#define OPTION_PROVIDER_NAME_DESC "The AI service provider."

#define OPTION_PROVIDER_KEY "key"
#define OPTION_PROVIDER_KEY_DESC "API Key value from the service provider."

#define OPTION_SERVICES "services"
#define OPTION_SERVICES_DESC "The AIservices offered by the provider."

#define OPTION_SERVICE_NAME "name"
#define OPTION_SERVICE_NAME_DESC "The name of the AI service."

#define OPTION_INSIGHT_COLUMN "textcolumn"
#define OPTION_INSIGHT_COLUMN_DESC "The column to be used for insights."

#define OPTION_SERVICE_PROMPT "prompt"
#define OPTION_SERVICE_PROMPT_DESC "The string to be used as input to the LLM."

#define OPTION_SERVICE_PROMPT_AGG "promptagg"
#define OPTION_SERVICE_PROMPT_AGG_DESC "The prompt used for the get_insight_agg() function."

#define OPTION_STORE_NAME "store_name"
#define OPTION_STORE_NAME_DESC "Table name having the materialised data."

#define OPTION_SQL_QUERY "query"
#define OPTION_SQL_QUERY_DESC "The SQL query(syntactically correct) to materialize."

#define OPTION_NL_QUERY "prompt"
#define OPTION_NL_QUERY_DESC "Prompt(natural language) to fetch matching data in the store."

#define OPTION_RECORD_COUNT "limit"
#define OPTION_RECORD_COUNT_DESC "No of records to display.(default: 2)"

#define OPTION_MATCHING_ALGORITHM "algorithm"
#define OPTION_MATCHING_ALGORITHM_DESC "Vector matching algorithm.(default/supported: cosine_similarity)"
/* ---------------------Function Arguments -->8------ */

#endif /* _AI_CONFIG_H_*/
