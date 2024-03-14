#ifndef _OPENAI_CONFIG_H_
#define _OPENAI_CONFIG_H_

/* -----------------8< gpt---------- */
/* "instruct" is better at answering pointed questions and "gpt" in general is chatty */
#define  MODEL_OPENAI_GPT "gpt-3.5-turbo-instruct"
#define  MODEL_OPENAI_GPT_DESCRIPTION "GPT Model better at answering pointed questions."

#define GPT_API_URL "https://api.openai.com/v1/completions"
#define GPT_SUMMARY_PROMPT "Get summary of the following in 1 lines."
#define GPT_AGG_PROMPT "Suggest a topic for the following."

#define RESPONSE_JSON_CHOICE "choices"
#define RESPONSE_JSON_KEY "text"

#define GPT_HELP "\nFunctions:\n"\
"(i) pg_ai_insight(<text column>,\n\
                   '<prompt> eg: Get summary of the following in 1 line')  \n\n" \
"(ii) pg_ai_insight_agg(<text column>,\n\
                        '<prompt agg> eg: Choose a topic for the following:') \n"
/* -----------------gpt >8---------- */


/* -----------------8< embeddings ---------- */
#define  MODEL_OPENAI_EMBEDDINGS "ada"
#define  MODEL_OPENAI_EMBEDDINGS_DESCRIPTION "OpenAI's embeddings model(vectors)"

#define EMBEDDINGS_API_URL "https://api.openai.com/v1/embeddings"
#define EMBEDDINGS_DESCRIPTION "OpenAI's embeddings model(vectors)"

#define EMBEDDINGS_HELP "\nFunctions:\n"\
"(ii) pg_ai_create_vector_store(store => '<new store name>', \n\
                          query => 'SQL query from which the store is made.', \n\
                          prompt => '<natural language prompt>' )\n\n" \
"(ii) pg_ai_query_vector_store(store => '<new store name>', \n\
                            prompt => '<natural language prompt>', \n\
                            count => <count of records to fetch>')\n"

#define SQL_QUERY_MAX_LENGTH 256*1024
/*  seems const - TODO */
#define EMBEDDINGS_LIST_SIZE 1536

#define EMBEDDINGS_COLUMN_NAME "embeddings"
#define PK_SUFFIX	"_id"
#define EMBEDDINGS_COSINE_SIMILARITY "cosine_similarity"

#define PG_EXTENSION_PG_VECTOR "vector"

#define MIN_COUNT_RECORDS 1
#define MAX_COUNT_RECORDS 10
/* ----------------- embeddings >8---------- */

/*--------------8< Image Gen --------------*/
#define  MODEL_OPENAI_IMAGE_GEN "dall-e2"
#define  MODEL_OPENAI_IMAGE_GEN_DESCRIPTION "OpenAI's text-to-image model"
#define IMAGE_GEN_AGG_PROMPT "Make a picture with the following"
#define IMAGE_GEN_PROMPT "Make a picture with the following"
#define IMAGE_GEN_API_URL "https://api.openai.com/v1/images/generations"

#define RESPONSE_JSON_DATA "data"
#define RESPONSE_JSON_URL "url"

#define IMAGE_GEN_HELP "\nFunctions:\n"\
"(i)  pg_ai_generate_image(<text column>,\n\
                           '<prompt> eg:Make a picture of the following:')\n\n" \
"(ii) pg_ai_generate_image_agg(<text column>,\n\
                      '<prompt agg> eg: Make a picture with the following:') \n"
/*-------------- Image Gen >8--------------*/


/* -----------------8< moderation ---------- */
/* "instruct" is better at answering pointed questions and "gpt" in general is chatty */
#define  MODEL_OPENAI_MODERATION "text-moderation-latest"
#define  MODEL_OPENAI_MODERATION_DESCRIPTION "Model that classifies input as potentially harmful across several categories."

#define MODERATION_API_URL "https://api.openai.com/v1/moderations"

#define MODERATION_HELP "\nFunctions:\n"\
"(i) pg_ai_moderation(<text column>)  \n\n" \
"(ii) pg_ai_moderation_agg(<text column>) \n"
/* -----------------moderation >8---------- */


#endif							/* _OPENAI_CONFIG_H_ */
