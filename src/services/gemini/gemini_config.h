#ifndef GEMINI_CONFIG_H
#define GEMINI_CONFIG_H

/* -----------------8< generate content ---------- */
#define MODEL_GEMINI_GENC_NAME "gemini-pro"
#define MODEL_GEMINI_GENC_DESCRIPTION                                          \
	"GPT Model for answering pointed questions."

#define GENC_API_URL                                                           \
	"https://generativelanguage.googleapis.com/v1beta/models/"                 \
	"gemini-pro:generateContent?key="

#define GENC_SUMMARY_PROMPT "Get summary of the following in 1 lines."
#define GENC_AGG_PROMPT "Suggest a topic for the following."

#define RESPONSE_JSON_CHOICE "choices"
#define RESPONSE_JSON_KEY "text"

#define GEN_CONTENT_HELP INSIGHT_FUNCTIONS
/* -----------------generate content >8---------- */

#endif /* GEMINI_CONFIG_H */