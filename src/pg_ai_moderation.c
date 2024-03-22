#include <postgres.h>
#include <funcapi.h>
#include <utils/builtins.h>

#include "rest/rest_transfer.h"
#include "ai_service.h"
#include "guc/pg_ai_guc.h"

/*
 * The implementation of SQL FUNCTION get_insight. Refer to the .sql file for
 * details on the parameters and return values.
 */
PG_FUNCTION_INFO_V1(pg_ai_moderation);
Datum pg_ai_moderation(PG_FUNCTION_ARGS)
{
	AIService *ai_service = palloc0(sizeof(AIService));
	int return_value;
	MemoryContext func_context;
	MemoryContext old_context;
	text *return_text;

	/* check for the column name whose value is to be interpreted */
	if (PG_ARGISNULL(0))
		ereport(ERROR,
				(errmsg("Incorrect parameters: please specify the column \
						 name\n")));

	/* Create a new memory context for this PgAi function */
	func_context = AllocSetContextCreate(
		CurrentMemoryContext, "ai functions context", ALLOCSET_DEFAULT_SIZES);
	old_context = MemoryContextSwitchTo(func_context);
	ai_service->memory_context = func_context;

	/* set the function specific flag */
	ai_service->function_flags |= FUNCTION_MODERATION;
	return_value =
		initialize_service(SERVICE_OPENAI, MODEL_OPENAI_MODERATION, ai_service);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

	/* set options based on parameters and read from guc */
	return_value = (ai_service->set_and_validate_options)(ai_service, fcinfo);
	if (return_value)
		ereport(ERROR, (errmsg("Error: Invalid Options\n")));

	/* set the service data to be sent to the AI service	*/
	return_value = (ai_service->set_service_data)(
		ai_service, text_to_cstring(PG_GETARG_TEXT_P(0)));
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot set \
										 service data"));

	/* prepare for transfer */
	return_value = (ai_service->prepare_for_transfer)(ai_service);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot set \
										 transfer data"));

	/* call the transfer */
	(ai_service->rest_transfer)(ai_service);

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
	int return_value;

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
		return_value = initialize_service(SERVICE_OPENAI,
										  MODEL_OPENAI_MODERATION, ai_service);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));
		ai_service->service_data->request[0] = '\0';

		return_value =
			(ai_service->set_and_validate_options)(ai_service, fcinfo);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Error: Invalid Options"));

		MemoryContextSwitchTo(old_context);
		fcinfo->flinfo->fn_extra = ai_service;
	}

	/* accumulate non NULL values */
	if (!PG_ARGISNULL(1))
	{
		/* set the service data to be sent to the AI service	*/
		return_value = (ai_service->set_service_data)(
			ai_service, text_to_cstring(PG_GETARG_TEXT_P(1)));
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot set \
											 service data"));
	}

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
	int return_value;
	text *return_text;

	ai_service = (AIService *)PG_GETARG_POINTER(0);
	if (ai_service == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Internal Error"));

	/* prepare for transfer */
	return_value = (ai_service->prepare_for_transfer)(ai_service);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot set \
										 transfer data"));

	/* call the transfer */
	(ai_service->rest_transfer)(ai_service);

	return_text = cstring_to_text((char *)(ai_service->rest_response->data));

	PG_RETURN_TEXT_P(return_text);
}
