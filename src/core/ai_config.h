#ifndef _AI_CONFIG_H_
#define _AI_CONFIG_H_

#include <stdio.h>
#include <curl/curl.h>
#include "service_option.h"
#include "services/openai/openai_config.h"
#include "services/gemini/gemini_config.h"

/* supported service providers */
/*--------------8<--------------*/
#define SERVICE_OPENAI_NAME "OpenAI"
#define SERVICE_OPENAI_DESCRIPTION "Models supported by OpenAI"
#define SERVICE_GEMINI_NAME "Gemini"
#define SERVICE_GEMINI_DESCRIPTION "Models supported by Gemini"
#define SERVICE_UNSUPPORTED_MSG "Service is not supported."
/*-------------->8--------------*/

/*------------8< Services. Models, Functions ----------*/
/* every supported service */
#define SERVICE_OPENAI 0x00000001
#define SERVICE_GEMINI 0x00000002

/* models supported by OpenAI */
#define MODEL_OPENAI_GPT 0x00000001
#define MODEL_OPENAI_EMBEDDINGS 0x00000002
#define MODEL_OPENAI_MODERATION 0x00000004
#define MODEL_OPENAI_IMAGE_GEN 0x00000008

/* models supported by Gemini */
#define MODEL_GEMINI_GENC 0x00000001

/* functions are common across services and models */
#define FUNCTION_GET_INSIGHT 0x00000001
#define FUNCTION_GET_INSIGHT_AGGREGATE 0x00000002
#define FUNCTION_GENERATE_IMAGE 0x00000004
#define FUNCTION_GENERATE_IMAGE_AGGREGATE 0x00000008
#define FUNCTION_CREATE_VECTOR_STORE 0x00000010
#define FUNCTION_QUERY_VECTOR_STORE 0x00000020
#define FUNCTION_MODERATION 0x00000040
#define FUNCTION_MODERATION_AGGREGATE 0x00000080
/*------------ Services. Models, Functions >8----------*/

/* -----------------8<--Function Arguments ---------- */
#define OPTION_SERVICE_NAME "service"
#define OPTION_SERVICE_NAME_DESC "The name of the AI service."

#define OPTION_MODEL_NAME "model"
#define OPTION_MODEL_NAME_DESC "The name of the model to be used."

#define OPTION_SERVICE_API_KEY "key"
#define OPTION_SERVICE_API_KEY_DESC "API Key value from the service provider."

#define OPTION_COLUMN_VALUE "column_name"
#define OPTION_COLUMN_VALUE_DESC "The input column to the LLM."

#define OPTION_SERVICE_PROMPT "prompt"
#define OPTION_SERVICE_PROMPT_DESC "Text to be used as input to the LLM."

#define OPTION_SERVICE_PROMPT_AGG "promptagg"
#define OPTION_SERVICE_PROMPT_AGG_DESC                                         \
	"The prompt to be used as input for the aggregate function."

#define OPTION_STORE_NAME "store"
#define OPTION_STORE_NAME_DESC                                                 \
	"Table name, will contain the query result set and vectors."

#define OPTION_SQL_QUERY "sql_query"
#define OPTION_SQL_QUERY_DESC "SQL query to materialize."

#define OPTION_NL_QUERY "nl_query"
#define OPTION_NL_QUERY_DESC                                                   \
	"Natural language query to fetch data from the vector store."

#define OPTION_NL_NOTES "notes"
#define OPTION_NL_NOTES_DESC "Notes on the result set."

#define OPTION_RECORD_COUNT "count"
#define OPTION_RECORD_COUNT_DESC "No of records to display.(default: 2)"

#define OPTION_SIMILARITY_ALGORITHM "algorithm"
#define OPTION_SIMILARITY_ALGORITHM_DESC                                       \
	"Vector similarity algorithm.(cosine(default), ecludian, inner_product)"

#define OPTION_ENDPOINT_URL "endpoint_url"
#define OPTION_ENDPOINT_URL_DESC "URL for the Rest API endpoint."
/* ---------------------Function Arguments -->8------ */

#define MAX_BYTE_VALUE 255

/* buffer sizes */
#define SERVICE_MAX_REQUEST_SIZE (1 * 1024 * 1024)
#define SERVICE_MAX_RESPONSE_SIZE (1 * 1024 * 1024)

/* default array lengths used within pg_ai */
#define PG_AI_NAME_LENGTH MAX_BYTE_VALUE
#define PG_AI_DESC_LENGTH MAX_BYTE_VALUE
#define COLUMN_NAME_LEN MAX_BYTE_VALUE
#define ERROR_MSG_LEN MAX_BYTE_VALUE

/* service data sizes */
#define SERVICE_DATA_SIZE (1 * 1024)

/* http responses */
#define HTTP_OK 200

/* internal error codes, for use within pg_ai */
#define RETURN_ZERO 0
#define RETURN_ERROR 1
#define RETURN_NETWORK_ERROR 2
/* #define RETURN_OOM 3 */
#define RETURN_OUT_OF_BOUNDS 4

/* limit to read the json into a buffer */
#define OPTIONS_FILE_SIZE 64 * 1024

/* help buffer size */
#define MAX_HELP_TEXT_SIZE 4 * 1024

/* TODO this has be replaced by dynamic READ call back */
#define POST_DATA_SIZE 16 * 1024

#define ERROR_PREFIX "PG_AI Error"
#define TRANSFER_FAIL_MSG "Transfer call failed. Try again."
#define BIG_DATA_FAIL_MSG "Data to big, model only supports %lu words."

#define APPROX_WORDS_PER_1K_TOKENS 400

#define PG_AI_DEBUG_0 0
#define PG_AI_DEBUG_1 1
#define PG_AI_DEBUG_2 2
#define PG_AI_DEBUG_3 3

#define PG_AI_MCTX "pg_ai_memory_context"

#define is_debug_level(level) (ai_service->debug_level >= level)

#define BYTE char

#define MAGIC_INT32 0xDEADBEEF

#define INSIGHT_FUNCTIONS                                                      \
	"\nFunctions:\n"                                                           \
	"(i) pg_ai_insight(<column_name>,\n"                                       \
	"                  '<prompt> eg: Get summary of the following in 1 "       \
	"line')\n\n"                                                               \
	"(ii) pg_ai_insight_agg(<col_name>,\n"                                     \
	"                       '<prompt agg> eg: Choose a topic for the "         \
	"following:')\n"

#endif /* _AI_CONFIG_H_ */
