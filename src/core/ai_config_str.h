#ifndef AI_CONFIG_STR_H
#define AI_CONFIG_STR_H

/* supported service providers */
/*--------------8<--------------*/
#define SERVICE_OPENAI_NAME "OpenAI"
#define SERVICE_OPENAI_DESCRIPTION "Models supported by OpenAI"
#define SERVICE_GEMINI_NAME "Gemini"
#define SERVICE_GEMINI_DESCRIPTION "Models supported by Gemini"
#define SERVICE_UNSUPPORTED_MSG "Service is not supported."
/*-------------->8--------------*/

/* string to describe the newly allocated memory context */
#define PG_AI_MCTX "pg_ai_memory_context"

/* column name consts for the vector store table */
#define EMBEDDINGS_COLUMN_NAME "embeddings"
#define PK_SUFFIX "_id"

/* name of the pgvector extension */
#define PG_EXTENSION_PG_VECTOR "vector"

/*---------8< supported similarity algos ----------------*/
#define EMBEDDINGS_SIMILARITY_COSINE "cosine"
#define EMBEDDINGS_SIMILARITY_EUCLIDEAN "euclidean"
#define EMBEDDINGS_SIMILARITY_INNER_PRODUCT "inner_product"
/*--------- supported similarity algos >8----------------*/

/*---------------------------8< help text ------------------------------------*/
#define INSIGHT_FUNCTIONS                                                      \
	"\nFunctions:\n"                                                           \
	"(i) pg_ai_insight(<column_name>,\n"                                       \
	"                  '<prompt> eg: Get summary of the following in 1 "       \
	"line')\n\n"                                                               \
	"(ii) pg_ai_insight_agg(<col_name>,\n"                                     \
	"                       '<prompt agg> eg: Choose a topic for the "         \
	"following:')\n"

#define MODERATION_FUNCTIONS                                                   \
	"\nFunctions:\n"                                                           \
	"(i) pg_ai_moderation(<column_name>, <prompt: additional text>)  \n\n"     \
	"(ii) pg_ai_moderation_agg(<column_name>, <promptagg: additional text>) "  \
	"\n"

#define EMBEDDING_FUNCTIONS                                                    \
	"\nFunctions:\n"                                                           \
	"(i) pg_ai_create_vector_store(store => '<new store name>', \n"            \
	"                               sql_query => 'SQL query from which the "   \
	"store is made.', \n"                                                      \
	"                               notes => '<notes on the result set>' "     \
	")\n\n"                                                                    \
	"(ii) pg_ai_query_vector_store(store => '<new store name>', \n"            \
	"                              nl_query => '<natural language prompt>', "  \
	"\n"                                                                       \
	"                              count => <count of records to "             \
	"fetch>')\n"
/*--------------------------- help text >8------------------------------------*/

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

#endif /* AI_CONFIG_STR_H */