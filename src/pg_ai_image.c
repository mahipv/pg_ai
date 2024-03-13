#include <postgres.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/typcache.h>
#include "executor/spi.h"
#include "rest/rest_transfer.h"
#include "ai_service.h"
#include "guc/pg_ai_guc.h"


PG_FUNCTION_INFO_V1(pg_ai_generate_image);
Datum
pg_ai_generate_image(PG_FUNCTION_ARGS)
{
	NameData	service_name;
	NameData	model_name;
	AIService  *ai_service;
	int			return_value;
	MemoryContext func_context;
	MemoryContext old_context;
	text	   *return_text;

	/* check for the column name whose value is to be interpreted */
	if (PG_ARGISNULL(0))
		ereport(ERROR,
				(errmsg("Incorrect parameters: please specify the column name\n")));

	func_context = AllocSetContextCreate(CurrentMemoryContext,
										 "ai functions context",
										 ALLOCSET_DEFAULT_SIZES);
	old_context = MemoryContextSwitchTo(func_context);
	ai_service = palloc0(sizeof(AIService));
	ai_service->memory_context = func_context;
	MemoryContextSwitchTo(old_context);

	/* default service an model */
	/* TODO: get the service and model from the guc */
	strcpy(service_name.data, SERVICE_OPENAI);
	strcpy(model_name.data, MODEL_OPENAI_IMAGE_GEN);

	ai_service->function_flags |= FUNCTION_GENERATE_IMAGE;
	return_value = initialize_service(NameStr(service_name), NameStr(model_name), ai_service);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

	/* set options based on parameters and read from guc */
	return_value = (ai_service->set_and_validate_options) (ai_service, fcinfo);
	if (return_value)
		ereport(ERROR, (errmsg("Error: Invalid Options\n")));

	/* initialize the service data to be sent to the AI service	*/
	return_value = (ai_service->init_service_data) (NULL, ai_service, NULL);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot make options"));

	/* print_service_options(ai_service->service_data->options, 0); */
	/* print_service_options(ai_service->service_data->options, 1); */

	/* call the transfer */
	(ai_service->rest_transfer) (ai_service);

	/* copy the return text so that the memory context can be freed */
	return_text = cstring_to_text((char *) (ai_service->rest_response->data));

	if (ai_service->memory_context)
		MemoryContextDelete(ai_service->memory_context);

	PG_RETURN_TEXT_P(return_text);
}

/* structure to save the context for the aggregate function */
typedef struct AggStateStruct
{
	AIService  *ai_service;
	char		column_data[16 * 1024];
	char	   *param_file_path;
}			AggStateStruct;

/*
 * The implementation of the aggregate transfer function, called once per
 * row. Refer the SQL FUNCTION _get_insight_agg_transfn in the .sql file for
 * details on the parameters and return value.
 *
 */
PG_FUNCTION_INFO_V1(pg_ai_generate_image_agg_transfn);
Datum
pg_ai_generate_image_agg_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext agg_context;
	MemoryContext old_context;
	AggStateStruct *state_struct;
	NameData	service_name;
	NameData	model_name;
	int			return_value;

	if (!AggCheckCallContext(fcinfo, &agg_context))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("transfn called in non-aggregate context")));

	if (!AggGetAggref(fcinfo))
		elog(ERROR, "aggregate called in a non-aggregate context");

	state_struct = fcinfo->flinfo->fn_extra;
	/* if this is the first call, set up the query state */
	if (state_struct == NULL)
	{
		/* allocate and initialize the query state */
		old_context = MemoryContextSwitchTo(agg_context);
		state_struct = (AggStateStruct *) palloc0(sizeof(AggStateStruct));
		*state_struct->column_data = '\0';
		state_struct->ai_service = (AIService *) palloc0(sizeof(AIService));

		/* default service an model */
		/* TODO: get the service and model from the guc */
		strcpy(service_name.data, SERVICE_OPENAI);
		strcpy(model_name.data, MODEL_OPENAI_IMAGE_GEN);

		state_struct->ai_service->function_flags |= FUNCTION_GENERATE_IMAGE_AGGREGATE;
		return_value = initialize_service(NameStr(service_name), NameStr(model_name), state_struct->ai_service);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

		return_value = (state_struct->ai_service->set_and_validate_options)
			(state_struct->ai_service, fcinfo);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Error: Invalid Options"));

		MemoryContextSwitchTo(old_context);
		fcinfo->flinfo->fn_extra = state_struct;
	}

	/* accumulate non NULL values */
	if (!PG_ARGISNULL(1))
	{
		strcat(state_struct->column_data, TextDatumGetCString(PG_GETARG_DATUM(1)));
		strcat(state_struct->column_data, " ");
		/* ereport(INFO,(errmsg("%s\n",state_struct->column_data))); */
	}

	PG_RETURN_POINTER(state_struct);
}

/*
 * The implementation of the aggregate final function, called only once after
 * all the rows are read. Refer the SQL FUNCTION _get_insight_agg_finalfn in
 * the .sql file for details on the parameters and return value.
 *
 */
PG_FUNCTION_INFO_V1(pg_ai_generate_image_agg_finalfn);
Datum
pg_ai_generate_image_agg_finalfn(PG_FUNCTION_ARGS)
{
	MemoryContext agg_context;
	MemoryContext old_context;
	AggStateStruct *state_struct;
	int			return_value;

	if (!AggCheckCallContext(fcinfo, &agg_context))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("called in non-aggregate context")));

	state_struct = (AggStateStruct *) PG_GETARG_POINTER(0);
	if (state_struct == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Internal Error"));

	if (state_struct->ai_service == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Internal Error"));

	/* the service was not initialized */
	if (state_struct->ai_service->get_service_name == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

	old_context = MemoryContextSwitchTo(agg_context);

	return_value = (state_struct->ai_service->init_service_data)
		(NULL, state_struct->ai_service, state_struct->column_data);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot make options"));

	/* ereport(INFO,(errmsg("Final :%s\n",state_struct->column_data))); */
	/* call the transfer */
	(state_struct->ai_service->rest_transfer) (state_struct->ai_service);

	MemoryContextSwitchTo(old_context);
	PG_RETURN_TEXT_P(cstring_to_text((char *) (state_struct->ai_service->rest_response->data)));
}
