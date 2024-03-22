#include <postgres.h>
#include <funcapi.h>

#include "guc/pg_ai_guc.h"

#define PG_AI_MIN_PG_VERSION 160000
#if PG_VERSION_NUM < PG_AI_MIN_PG_VERSION
#error "Unsupported PostgreSQL version."
#endif

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

void _PG_init(void) { define_pg_ai_guc_variables(); }

void _PG_fini(void) {}
