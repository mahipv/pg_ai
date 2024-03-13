#ifndef _PG_AI_GUC_H
#define _PG_AI_GUC_H

#define PG_AI_GUC_API_KEY "pg_ai.api_key"
#define PG_AI_GUC_API_KEY_DESCRIPTION "AI Service API key"

#define PG_AI_GUC_MODEL "pg_ai.ai_service"
#define PG_AI_GUC_MODEL_DESCRIPTION "AI Service"

#define PG_AI_GUC_SERVICE "pg_ai.openai_model"
#define PG_AI_GUC_SERVICE_DESCRIPTION "OpenAI model"

#define PG_AI_GUC_VALUE_LEN 255

void		define_pg_ai_guc_variables(void);
char	   *get_pg_ai_guc_variable(char *name);

#endif/* _PG_AI_GUC_H_ */