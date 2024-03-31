#ifndef GEMINI_CONFIG_H
#define GEMINI_CONFIG_H

/* -----------------8< generate content ---------- */
#define MODEL_GEMINI_GENC_NAME "gemini-pro"
#define MODEL_GEMINI_GENC_DESCRIPTION "Gemini content generation model."

#define GENC_API_URL                                                           \
	"https://generativelanguage.googleapis.com/v1beta/models/"                 \
	"gemini-pro:generateContent?key="

#define GENC_SUMMARY_PROMPT "Get summary of the following in 1 lines."
#define GENC_AGG_PROMPT "Suggest a topic for the following."

#define GEN_CONTENT_HELP INSIGHT_FUNCTIONS
/* -----------------generate content >8---------- */

/* -----------------8< generate content mod ---------- */
#define MODEL_GEMINI_GENC_MOD_NAME "gemini-pro"
#define MODEL_GEMINI_GENC_MOD_DESCRIPTION                                      \
	"Gemini content generation model with moderation."

#define GENC_MOD_API_URL                                                       \
	"https://generativelanguage.googleapis.com/v1beta/models/"                 \
	"gemini-pro:generateContent?key="

#define GENC_MOD_SUMMARY_PROMPT "Give feedback without text response:"
#define GENC_MOD_AGG_PROMPT "Give feedback without text response:"

#define GENC_MOD_CONTENT_HELP MODERATION_FUNCTIONS
/* -----------------generate content mod >8---------- */

/* -----------------8< gen eembeddings service ---------- */
#define MODEL_GEMINI_EMBEDDINGS_NAME "embedding-001"
#define MODEL_GEMINI_EMBEDDINGS_DESCRIPTION "Geminis' embedding model(vectors)"

#define GEMINI_EMBEDDINGS_API_URL                                              \
	"https://generativelanguage.googleapis.com/v1beta/models/"                 \
	"embedding-001:batchEmbedContents?key="

#define GEMINI_EMBEDDINGS_HELP EMBEDDING_FUNCTIONS

/*  seems const - TODO */
#define GEMINI_EMBEDDINGS_LIST_SIZE 768
/* ----------------- gen eembeddings service >8---------- */

#endif /* GEMINI_CONFIG_H */