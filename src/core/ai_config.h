#ifndef _AI_CONFIG_H_
#define _AI_CONFIG_H_

#include <stdio.h>
#include <curl/curl.h>

#include "ai_config_str.h"
#include "service_option.h"
#include "services/openai/openai_config.h"
#include "services/gemini/gemini_config.h"

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
#define MODEL_GEMINI_GENC_MOD 0x00000002
#define MODEL_GEMINI_EMBEDDINGS 0x00000004

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

#define APPROX_WORDS_PER_1K_TOKENS 400

#define PG_AI_DEBUG_0 0
#define PG_AI_DEBUG_1 1
#define PG_AI_DEBUG_2 2
#define PG_AI_DEBUG_3 3

#define DEBUG_LEVEL(level) (ai_service->debug_level >= level)

#define BYTE char

#define MAGIC_INT32 0xDEADBEEF

#define MIN_COUNT_RECORDS 1
#define MAX_COUNT_RECORDS 10

/* max array size to hold the SQL queries */
#define SQL_QUERY_MAX_LENGTH 256 * 1024

#endif /* _AI_CONFIG_H_ */
