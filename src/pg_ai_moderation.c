#include <postgres.h>
#include <funcapi.h>
#include <utils/builtins.h>

#include "core/ai_service.h"

/*
 * The implementation of SQL FUNCTION get_insight. Refer to the .sql file for
 * details on the parameters and return values.
 */
PG_FUNCTION_INFO_V1(pg_ai_moderation);
Datum pg_ai_moderation(PG_FUNCTION_ARGS)
{
	AIService *ai_service = palloc0(sizeof(AIService));
	MemoryContext func_context;
	MemoryContext old_context;
	text *return_text;

	/* check for the column to be interpreted */
	if (PG_ARGISNULL(0))
		PG_RETURN_TEXT_P(GET_ERR_TEXT(NULL_STR));

	/* Create a new memory context for this PgAi function */
	func_context = AllocSetContextCreate(CurrentMemoryContext, PG_AI_MCTX,
										 ALLOCSET_DEFAULT_SIZES);
	old_context = MemoryContextSwitchTo(func_context);
	ai_service->memory_context = func_context;

	/* set the function specific flag */
	ai_service->function_flags |= FUNCTION_MODERATION;
	INITIALIZE_SERVICE(SERVICE_OPENAI, MODEL_OPENAI_MODERATION, ai_service);

	/* set options based on parameters and read from guc */
	SET_AND_VALIDATE_OPTIONS(ai_service, fcinfo);

	/* set the service data to be sent to the AI service	*/
	SET_SERVICE_DATA(ai_service, text_to_cstring(PG_GETARG_TEXT_P(0)));

	/* prepare for transfer */
	PREPARE_FOR_TRANSFER(ai_service);

	/* call the transfer */
	REST_TRANSFER(ai_service);

	/* copy the result to old mem conext and free the function context */
	MemoryContextSwitchTo(old_context);
	return_text = cstring_to_text((char *)(ai_service->rest_response->data));
	if (ai_service->memory_context)
		MemoryContextDelete(ai_service->memory_context);
	pfree(ai_service);

	PG_RETURN_TEXT_P(return_text);
}

/*
 * The implementation of the aggregate transfer function, called once per
 * row. Refer the SQL FUNCTION _get_insight_agg_transfn in the .sql file for
 * details on the parameters and return value.
 *
 */
PG_FUNCTION_INFO_V1(pg_ai_moderation_agg_transfn);
Datum pg_ai_moderation_agg_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext old_context;
	MemoryContext agg_context;
	AIService *ai_service;

	if (!AggCheckCallContext(fcinfo, &agg_context))
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("Function called in non-aggregate context")));

	ai_service = fcinfo->flinfo->fn_extra;
	/* if this is the first call, set up the query state */
	if (ai_service == NULL)
	{
		old_context = MemoryContextSwitchTo(agg_context);
		ai_service = (AIService *)palloc0(sizeof(AIService));
		ai_service->memory_context = agg_context;

		ai_service->function_flags |= FUNCTION_MODERATION_AGGREGATE;
		/* get these settings from guc */
		INITIALIZE_SERVICE(SERVICE_OPENAI, MODEL_OPENAI_MODERATION, ai_service);
		ai_service->service_data->request[0] = '\0';

		SET_AND_VALIDATE_OPTIONS(ai_service, fcinfo);

		MemoryContextSwitchTo(old_context);
		fcinfo->flinfo->fn_extra = ai_service;
	}

	/* accumulate non NULL values set the service data */
	if (!PG_ARGISNULL(1))
		SET_SERVICE_DATA(ai_service, text_to_cstring(PG_GETARG_TEXT_P(1)));

	PG_RETURN_POINTER(ai_service);
}

/*
 * The implementation of the aggregate final function, called only once after
 * all the rows are read. Refer the SQL FUNCTION _get_insight_agg_finalfn in
 * the .sql file for details on the parameters and return value.
 *
 */
PG_FUNCTION_INFO_V1(pg_ai_moderation_agg_finalfn);
Datum pg_ai_moderation_agg_finalfn(PG_FUNCTION_ARGS)
{
	AIService *ai_service;
	text *return_text;

	/* ai_service will not created if all values of column are NULL */
	ai_service = (AIService *)PG_GETARG_POINTER(0);
	if (ai_service == NULL)
		PG_RETURN_TEXT_P(GET_ERR_TEXT(NULL_STR));

	/* prepare for transfer */
	PREPARE_FOR_TRANSFER(ai_service);

	/* call the transfer */
	REST_TRANSFER(ai_service);

	return_text = cstring_to_text((char *)(ai_service->rest_response->data));

	PG_RETURN_TEXT_P(return_text);
}
