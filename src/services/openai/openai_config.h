#ifndef _OPENAI_CONFIG_H_
#define _OPENAI_CONFIG_H_

/* -----------------8< gpt service ---------- */
/*
 * "instruct" is better at answering pointed questions and "gpt"
 * in general is chatty
 */
#define MODEL_OPENAI_GPT_NAME "gpt-3.5-turbo-instruct"
#define MODEL_OPENAI_GPT_DESCRIPTION                                           \
	"GPT Model for answering pointed questions."

#define GPT_API_URL "https://api.openai.com/v1/completions"
#define GPT_SUMMARY_PROMPT "Get summary of the following in 1 lines."
#define GPT_AGG_PROMPT "Suggest a topic for the following."

#define RESPONSE_JSON_CHOICE "choices"
#define RESPONSE_JSON_KEY "text"

#define GPT_HELP INSIGHT_FUNCTIONS

/* -----------------gpt service >8---------- */

/* -----------------8< embeddings service ---------- */
#define MODEL_OPENAI_EMBEDDINGS_NAME "text-embedding-ada-002"
#define MODEL_OPENAI_EMBEDDINGS_DESCRIPTION "OpenAI's embeddings model(vectors)"

#define EMBEDDINGS_API_URL "https://api.openai.com/v1/embeddings"

#define EMBEDDINGS_HELP EMBEDDING_FUNCTIONS

/*  seems const - TODO */
#define EMBEDDINGS_LIST_SIZE 1536
/* ----------------- embeddings service >8---------- */

/*--------------8< Image Gen service --------------*/
#define MODEL_OPENAI_IMAGE_GEN_NAME "dall-e-3"
#define MODEL_OPENAI_IMAGE_GEN_DESCRIPTION "OpenAI's text-to-image model"

#define IMAGE_GEN_PROMPT "Make a picture of the following"
#define IMAGE_GEN_AGG_PROMPT "Make a picture with the following"
#define IMAGE_GEN_API_URL "https://api.openai.com/v1/images/generations"

#define RESPONSE_JSON_DATA "data"
#define RESPONSE_JSON_URL "url"

#define IMAGE_GEN_HELP                                                         \
	"\nFunctions:\n"                                                           \
	"(i)  pg_ai_generate_image(<column_name>,\n"                               \
	"                           '<prompt> eg:Make a picture of the "           \
	"following:')\n\n"                                                         \
	"(ii) pg_ai_generate_image_agg(<column_name>,\n"                           \
	"                      '<prompt agg> eg: Make a picture with the "         \
	"following:')\n"
/*-------------- Image Gen service >8--------------*/

/* -----------------8< moderation service ---------- */
/* "instruct" is better at answering pointed questions and "gpt" in general is
 * chatty */
#define MODEL_OPENAI_MODERATION_NAME "text-moderation-latest"
#define MODEL_OPENAI_MODERATION_DESCRIPTION                                    \
	"Classifies input on harmful categories."

#define MODERATION_API_URL "https://api.openai.com/v1/moderations"

#define MODERATION_HELP MODERATION_FUNCTIONS
/* -----------------moderation service >8---------- */

#endif /* _OPENAI_CONFIG_H_ */
