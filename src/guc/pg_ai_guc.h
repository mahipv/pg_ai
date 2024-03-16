#ifndef _PG_AI_GUC_H
#define _PG_AI_GUC_H

/* ------8< string gucs ----------------------- */
#define PG_AI_GUC_API_KEY "pg_ai.api_key"
#define PG_AI_GUC_API_KEY_DESCRIPTION "AI Service API key"

#define PG_AI_GUC_MODEL "pg_ai.ai_service"
#define PG_AI_GUC_MODEL_DESCRIPTION "AI Service"

#define PG_AI_GUC_SERVICE "pg_ai.openai_model"
#define PG_AI_GUC_SERVICE_DESCRIPTION "OpenAI model"
/* ------ string gucs >8----------------------- */

/* ------8< integer gucs ----------------------- */
#define PG_AI_GUC_WORK_MEM_SIZE "pg_ai.work_mem"
#define PG_AI_GUC_WORK_MEM_SIZE_DESCRIPTION "Max memory that can be used by pg_ai in KB"
#define PG_AI_GUC_MINIMUM_WORK_MEM_KB 1024
#define PG_AI_GUC_DEFAULT_WORK_MEM_KB (8*1024)
#define PG_AI_GUC_MAXIMUM_WORK_MEM_KB (1024*1024)
/* ------ integer gucs >8----------------------- */

void		define_pg_ai_guc_variables(void);
char	   *get_pg_ai_guc_string_variable(char *name);
int		   *get_pg_ai_guc_int_variable(char *name);

#endif      /* PG_AI_GUC_H*/