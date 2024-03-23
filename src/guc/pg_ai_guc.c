#include "pg_ai_guc.h"

#include "postgres.h"
#include "utils/guc.h"

/* GUCs that accept a strings values */
typedef struct PgAiStringGUCs
{
	char *name;
	char *description;
} PgAiStringGUCs;

/* for new str GUCs, add entries to this and the values array */
PgAiStringGUCs pg_ai_str_gucs[] = {
	{PG_AI_GUC_API_KEY, PG_AI_GUC_API_KEY_DESCRIPTION},
	{PG_AI_GUC_MODEL, PG_AI_GUC_MODEL_DESCRIPTION},
	{PG_AI_GUC_SERVICE, PG_AI_GUC_SERVICE_DESCRIPTION},
	{PG_AI_GUC_VEC_SIMILARITY_ALGO, PG_AI_GUC_VEC_SIMILARITY_ALGO_DESC}};

/* the values array should be in sync with the above definition array */
static char *pg_ai_str_guc_values[] = {NULL, NULL, NULL, NULL};

/* GUCs that accept a integer values */
typedef struct PgAiIntGUCs
{
	char *name;
	char *description;
	int min_value;
	int max_value;
} PgAiIntGUCs;

/* for new int GUCs, add entries to this and the values array */
PgAiIntGUCs pg_ai_int_gucs[] = {
	{PG_AI_GUC_WORK_MEM_SIZE, PG_AI_GUC_WORK_MEM_SIZE_DESCRIPTION,
	 PG_AI_GUC_MINIMUM_WORK_MEM_KB, PG_AI_GUC_MAXIMUM_WORK_MEM_KB},
	{PG_AI_GUC_DEBUG_LEVEL, PG_AI_GUC_DEBUG_LEVEL_DESCRIPTION,
	 PG_AI_GUC_MINIMUM_DEBUG_LEVEL, PG_AI_GUC_MAXIMUM_DEBUG_LEVEL}};

/* set the default/boot value */
static int pg_ai_work_mem = PG_AI_GUC_DEFAULT_WORK_MEM_KB;
static int pg_ai_debug_level = PG_AI_GUC_DEFAULT_DEBUG_LEVEL;

/* the values array should be in sync with the above definition array */
static int *pg_ai_int_guc_values[] = {&pg_ai_work_mem, &pg_ai_debug_level};

/*
 * Define the GUCs for the AI services.
 */
void define_pg_ai_guc_variables()
{
	int count;

	/* Define the string GUCs */
	count = sizeof(pg_ai_str_gucs) / sizeof(pg_ai_str_gucs[0]);
	for (int i = 0; i < count; i++)
	{
		DefineCustomStringVariable(
			pg_ai_str_gucs[i].name,		   /* name */
			pg_ai_str_gucs[i].description, /* short desc */
			pg_ai_str_gucs[i].description, /* long desc */
			&pg_ai_str_guc_values[i],	   /* char** value */
			NULL,						   /* boot/default value */
			PGC_USERSET,				   /* context */
			0,							   /* flags */
			NULL,						   /* check_hook */
			NULL,						   /* assign_hook */
			NULL						   /* show_hook */
		);
	}

	/* Define the integer GUCs */
	count = sizeof(pg_ai_int_gucs) / sizeof(pg_ai_int_gucs[0]);
	for (int i = 0; i < count; i++)
	{
		DefineCustomIntVariable(
			pg_ai_int_gucs[i].name,		   /* name */
			pg_ai_int_gucs[i].description, /* short desc */
			pg_ai_int_gucs[i].description, /* long desc */
			pg_ai_int_guc_values[i],	   /* int* for value */
			*pg_ai_int_guc_values[i],	   /* boot/default value */
			pg_ai_int_gucs[i].min_value, pg_ai_int_gucs[i].max_value,
			PGC_USERSET, /* context */
			0,			 /* flags */
			NULL,		 /* check_hook */
			NULL,		 /* assign_hook */
			NULL		 /* show_hook */
		);
	}
}

/*
 * Rreturn the value of a given string GUC.
 */
char *get_pg_ai_guc_string_variable(char *name)
{
	int count;

	count = sizeof(pg_ai_str_gucs) / sizeof(pg_ai_str_gucs[0]);
	for (int i = 0; i < count; i++)
		if (strcmp(pg_ai_str_gucs[i].name, name) == 0)
			return pg_ai_str_guc_values[i];
	return NULL;
}

/*
 * Rreturn the value of a given integer GUC.
 */
int *get_pg_ai_guc_int_variable(char *name)
{
	int count;

	count = sizeof(pg_ai_int_gucs) / sizeof(pg_ai_int_gucs[0]);
	for (int i = 0; i < count; i++)
		if (strcmp(pg_ai_int_gucs[i].name, name) == 0)
			return pg_ai_int_guc_values[i];
	return NULL;
}
