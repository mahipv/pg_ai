#include "pg_ai_guc.h"
#include "postgres.h"
#include "utils/guc.h"

typedef struct PG_AI_STRING_GUCs
{
	char	   *name;
	char	   *description;
}			PG_AI_STRING_GUCs;
PG_AI_STRING_GUCs pg_ai_string_gucs[] =
{
	{PG_AI_GUC_API_KEY, PG_AI_GUC_API_KEY_DESCRIPTION},
	{PG_AI_GUC_MODEL, PG_AI_GUC_MODEL_DESCRIPTION},
	{PG_AI_GUC_SERVICE, PG_AI_GUC_SERVICE_DESCRIPTION},
};
static char *pg_ai_string_guc_values[] = {NULL, NULL, NULL};


typedef struct PG_AI_INT_GUCs
{
	char	   *name;
	char	   *description;
	int			min_value;
	int			max_value;
}			PG_AI_INT_GUCs;
PG_AI_INT_GUCs pg_ai_int_gucs[] =
{
	{PG_AI_GUC_WORK_MEM_SIZE, PG_AI_GUC_WORK_MEM_SIZE_DESCRIPTION, PG_AI_GUC_MINIMUM_WORK_MEM_KB, PG_AI_GUC_MAXIMUM_WORK_MEM_KB}
};
static int	pg_ai_work_mem = PG_AI_GUC_DEFAULT_WORK_MEM_KB;
static int *pg_ai_int_guc_values[] = {&pg_ai_work_mem};


void
define_pg_ai_guc_variables()
{

	int			count = sizeof(pg_ai_string_gucs) / sizeof(pg_ai_string_gucs[0]);

	for (int i = 0; i < count; i++)
	{
		DefineCustomStringVariable(pg_ai_string_gucs[i].name,	/* name */
								   pg_ai_string_gucs[i].description,	/* short desc */
								   pg_ai_string_gucs[i].description,	/* long desc */
								   &pg_ai_string_guc_values[i], /* char** for value */
								   NULL,	/* boot/default value */
								   PGC_USERSET, /* context */
								   0,	/* flags */
								   NULL,	/* check_hook */
								   NULL,	/* assign_hook */
								   NULL /* show_hook */
			);
	}

	count = sizeof(pg_ai_int_gucs) / sizeof(pg_ai_int_gucs[0]);
	for (int i = 0; i < count; i++)
	{
		DefineCustomIntVariable(pg_ai_int_gucs[i].name, /* name */
								pg_ai_int_gucs[i].description,	/* short desc */
								pg_ai_int_gucs[i].description,	/* long desc */
								pg_ai_int_guc_values[i],	/* int* for value */
								*pg_ai_int_guc_values[i],	/* boot/default value */
								pg_ai_int_gucs[i].min_value,
								pg_ai_int_gucs[i].max_value,
								PGC_USERSET,	/* context */
								0,	/* flags */
								NULL,	/* check_hook */
								NULL,	/* assign_hook */
								NULL	/* show_hook */
			);
	}
}

char *
get_pg_ai_guc_string_variable(char *name)
{
	int			count = sizeof(pg_ai_string_gucs) / sizeof(pg_ai_string_gucs[0]);

	for (int i = 0; i < count; i++)
		if (strcmp(pg_ai_string_gucs[i].name, name) == 0)
			return pg_ai_string_guc_values[i];
	return NULL;
}

int *
get_pg_ai_guc_int_variable(char *name)
{
	int			count = sizeof(pg_ai_int_gucs) / sizeof(pg_ai_int_gucs[0]);

	for (int i = 0; i < count; i++)
		if (strcmp(pg_ai_int_gucs[i].name, name) == 0)
			return pg_ai_int_guc_values[i];
	return NULL;
}
