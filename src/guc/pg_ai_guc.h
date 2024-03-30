#ifndef _PG_AI_GUC_H
#define _PG_AI_GUC_H

/* ------8< string gucs ----------------------- */
#define PG_AI_GUC_API_KEY "pg_ai.api_key"
#define PG_AI_GUC_API_KEY_DESCRIPTION "AI Service API key"

#define PG_AI_GUC_SERVICE "pg_ai.service"
#define PG_AI_GUC_SERVICE_DESCRIPTION "AI Service"

#define PG_AI_GUC_MODEL "pg_ai.model"
#define PG_AI_GUC_MODEL_DESCRIPTION "AI model"

#define PG_AI_GUC_VEC_SIMILARITY_ALGO "pg_ai.similarity_algorithm"
#define PG_AI_GUC_VEC_SIMILARITY_ALGO_DESC "Vector similarity algorithm"
/* ------ string gucs >8----------------------- */

/* ------8< integer gucs ----------------------- */
#define PG_AI_GUC_WORK_MEM_SIZE "pg_ai.work_mem"
#define PG_AI_GUC_WORK_MEM_SIZE_DESCRIPTION                                    \
	"Max memory that can be used by pg_ai in KB"
#define PG_AI_GUC_MINIMUM_WORK_MEM_KB 1024
#define PG_AI_GUC_DEFAULT_WORK_MEM_KB (8 * 1024)
#define PG_AI_GUC_MAXIMUM_WORK_MEM_KB (1024 * 1024)

#define PG_AI_GUC_DEBUG_LEVEL "pg_ai.debug_level"
#define PG_AI_GUC_DEBUG_LEVEL_DESCRIPTION                                      \
	"Debug level for pg_ai. 0: No debug, 1: Info, 2: Debug, 3: Trace"
#define PG_AI_GUC_MINIMUM_DEBUG_LEVEL 0
#define PG_AI_GUC_DEFAULT_DEBUG_LEVEL 1
#define PG_AI_GUC_MAXIMUM_DEBUG_LEVEL 3
/* ------ integer gucs >8----------------------- */

void define_pg_ai_guc_variables(void);
char *get_pg_ai_guc_string_variable(char *name);
int *get_pg_ai_guc_int_variable(char *name);

#endif /* _PG_AI_GUC_H */
