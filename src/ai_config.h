#ifndef _AI_CONFIG_H_
#define _AI_CONFIG_H_

#include <stdio.h>
#include <curl/curl.h>
#include "service_option.h"
#include "openai_services/openai_config.h"

/* supported service providers */
/*--------------8<--------------*/
#define  SERVICE_OPENAI "OpenAI"
#define  SERVICE_OPENAI_DESCRIPTION "AI services supported by OpenAI"
#define  SERVICE_UNSUPPORTED_MSG "Service is not supported."
/*-------------->8--------------*/

/* buffer sizes */
#define SERVICE_MAX_REQUEST_SIZE (1 * 1024 * 1024)
#define SERVICE_MAX_RESPONSE_SIZE (1 * 1024 * 1024)

/* default array lengths used within pg_ai */
#define PG_AI_NAME_LENGTH 80
#define PG_AI_DESC_LENGTH 255

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
#define OPTIONS_FILE_SIZE 64*1024

/* help buffer size */
#define MAX_HELP_SIZE 4*1024

/* TODO this has be replaced by dynamic READ call back */
#define POST_DATA_SIZE 16*1024

#define ERROR_PREFIX "PG_AI Error"
#define TRANSFER_FAIL_MSG "Transfer call failed. Try again."
#define BIG_DATA_FAIL_MSG "Data to big, model only supports %lu words."

#define COLUMN_NAME_LEN 255

#define APPROX_WORDS_PER_1K_TOKENS 400

/* every service function has a unique constant */
#define FUNCTION_GET_INSIGHT                0x00000001
#define FUNCTION_GET_INSIGHT_AGGREGATE      0x00000002
#define FUNCTION_GENERATE_IMAGE             0x00000004
#define FUNCTION_GENERATE_IMAGE_AGGREGATE   0x00000008
#define FUNCTION_CREATE_VECTOR_STORE        0x00000010
#define FUNCTION_QUERY_VECTOR_STORE         0x00000020


/* -----------------8<--Function Arguments ---------- */
#define OPTION_SERVICE_NAME "service"
#define OPTION_SERVICE_NAME_DESC "The name of the AI service."

#define OPTION_MODEL_NAME "model"
#define OPTION_MODEL_NAME_DESC "The name of the model to be used."

#define OPTION_SERVICE_API_KEY "key"
#define OPTION_SERVICE_API_KEY_DESC "API Key value from the service provider."

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

#endif							/* _AI_CONFIG_H_ */
