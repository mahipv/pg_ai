#include "pg_ai_guc.h"
#include "postgres.h"
#include "utils/guc.h"

typedef struct PGAIStringGUCs
{
	char	   *name;
	char	   *description;
}			PGAIStringGUCs;

PGAIStringGUCs pg_ai_guc_variables[] = {
	{PG_AI_GUC_API_KEY, PG_AI_GUC_API_KEY_DESCRIPTION},
	{PG_AI_GUC_MODEL, PG_AI_GUC_MODEL_DESCRIPTION},
	{PG_AI_GUC_SERVICE, PG_AI_GUC_SERVICE_DESCRIPTION},
};

static char *pg_ai_guc_values[] = {NULL, NULL, NULL};
static void
pg_ai_guc_variable_assign(const char *newval, void *extra)
{
	/* Validate and assign new value */
	if (newval != NULL)
	{
		/* Do any validation if necessary */
		/* For example, you might check the format or range */
	}
	ereport(LOG, (errmsg("my_guc_variable_assign: newval = %s", newval)));
	/* Assign the value */
}


void
define_pg_ai_guc_variables()
{
	int			count = sizeof(pg_ai_guc_variables) / sizeof(pg_ai_guc_variables[0]);

	for (int i = 0; i < count; i++)
	{
		DefineCustomStringVariable(pg_ai_guc_variables[i].name, /* name */
								   pg_ai_guc_variables[i].description,	/* short desc */
								   pg_ai_guc_variables[i].description,	/* long desc */
								   &pg_ai_guc_values[i],	/* char** for value */
								   NULL,	/* boot/default value */
								   PGC_USERSET, /* context */
								   0,	/* flags */
								   NULL,	/* check_hook */
								   pg_ai_guc_variable_assign,	/* assign_hook */
								   NULL /* show_hook */
			);
	}

}

char *
get_pg_ai_guc_variable(char *name)
{
	int			count = sizeof(pg_ai_guc_variables) / sizeof(pg_ai_guc_variables[0]);

	for (int i = 0; i < count; i++)
		if (strcmp(pg_ai_guc_variables[i].name, name) == 0)
			return pg_ai_guc_values[i];
	return NULL;
}
