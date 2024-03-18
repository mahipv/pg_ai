#include <postgres.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/typcache.h>
#include "executor/spi.h"
#include "rest/rest_transfer.h"
#include "ai_service.h"
#include "utils_pg_ai.h"
#include "guc/pg_ai_guc.h"

#define PG_AI_MIN_PG_VERSION 160000
#if PG_VERSION_NUM < PG_AI_MIN_PG_VERSION
#error "Unsupported PostgreSQL version."
#endif

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

void
_PG_init(void)
{
	define_pg_ai_guc_variables();
}

void
_PG_fini(void)
{

}

/*
 * The implementation of SQL FUNCTION get_insight. Refer to the .sql file for
 * details on the parameters and return values.
 */
PG_FUNCTION_INFO_V1(pg_ai_insight);
Datum
pg_ai_insight(PG_FUNCTION_ARGS)
{
	AIService  *ai_service = palloc0(sizeof(AIService));
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
	ai_service->memory_context = func_context;

	ai_service->function_flags |= FUNCTION_GET_INSIGHT;
	/* get these settings from guc */
	return_value = initialize_service(SERVICE_OPENAI, MODEL_OPENAI_GPT, ai_service);
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

	MemoryContextSwitchTo(old_context);
	return_text = cstring_to_text((char *) (ai_service->rest_response->data));
	if (ai_service->memory_context)
		MemoryContextDelete(ai_service->memory_context);
	pfree(ai_service);
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
PG_FUNCTION_INFO_V1(pg_ai_insight_agg_transfn);
Datum
pg_ai_insight_agg_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext agg_context;
	MemoryContext old_context;
	AggStateStruct *state_struct;
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

		state_struct->ai_service->function_flags |= FUNCTION_GET_INSIGHT_AGGREGATE;
		/* get these settings from guc */
		return_value = initialize_service(SERVICE_OPENAI, MODEL_OPENAI_GPT, state_struct->ai_service);
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
PG_FUNCTION_INFO_V1(pg_ai_insight_agg_finalfn);
Datum
pg_ai_insight_agg_finalfn(PG_FUNCTION_ARGS)
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


/*
 * The implementation of SQL FUNCTION create_vector_store. Refer to the .sql file for
 * details on the parameters and return values.
 */
PG_FUNCTION_INFO_V1(pg_ai_create_vector_store);
Datum
pg_ai_create_vector_store(PG_FUNCTION_ARGS)
{
	AIService  *ai_service;
	MemoryContext func_context;
	MemoryContext old_context;
	int			return_value;

	func_context = AllocSetContextCreate(CurrentMemoryContext,
										 "ai functions context",
										 ALLOCSET_DEFAULT_SIZES);
	old_context = MemoryContextSwitchTo(func_context);
	ai_service = palloc0(sizeof(AIService));
	ai_service->memory_context = func_context;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		ereport(FATAL,
				(errmsg("Incorrect parameters cannot proceed.\n")));

	ai_service->function_flags |= FUNCTION_CREATE_VECTOR_STORE;
	return_value = initialize_service(SERVICE_OPENAI, MODEL_OPENAI_EMBEDDINGS, ai_service);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

	return_value = (ai_service->set_and_validate_options) (ai_service, fcinfo);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Error: Invalid Options"));

	/* make call to ada service to create the vectore store */
	return_value = (ai_service->init_service_data) (NULL, ai_service, NULL);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot initialize data."));

	/* call the transfer TODO return value */
	(ai_service->rest_transfer) (ai_service);

	MemoryContextSwitchTo(old_context);
	PG_RETURN_TEXT_P(cstring_to_text((char *) (ai_service->rest_response->data)));
}

/*
 * The implementation of SQL FUNCTION query_vector_store. Refer to the .sql file for
 * details on the parameters and return values.
 */
typedef struct srf_query_data
{
	int			current_row;
	int			max_rows;
	bool		print_header_info;
	SPITupleTable *tuble_table;
}			srf_query_data;

PG_FUNCTION_INFO_V1(pg_ai_query_vector_store);
Datum
pg_ai_query_vector_store(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	MemoryContext oldcontext;
	srf_query_data *query_data;
	Datum		result;
	char	   *query_string;
	uint32_t	spi_result;
	int			i;
	int			col_count;
	HeapTuple	tuple;
	StringInfo	header;
	AIService  *ai_service;
	int			return_value;
	char		pk_col[COLUMN_NAME_LEN];
	char	   *hide_cols[] = {EMBEDDINGS_COLUMN_NAME, EMBEDDINGS_COSINE_SIMILARITY, pk_col};
	int			hide_col_count = sizeof(hide_cols) / sizeof(hide_cols[0]);

	if (SRF_IS_FIRSTCALL())
	{
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		query_string = palloc(SERVICE_MAX_RESPONSE_SIZE);

		/* --------------------------------------------------------------------- */
		/* make the reset call to get the embeddings and the corresponding sql */
		ai_service = palloc0(sizeof(AIService));


		ai_service->function_flags |= FUNCTION_QUERY_VECTOR_STORE;
		/* initialize based on the service and model */
		return_value = initialize_service(SERVICE_OPENAI, MODEL_OPENAI_EMBEDDINGS, ai_service);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

		/* pass on the arguments to the service to validate */
		return_value = (ai_service->set_and_validate_options) (ai_service, fcinfo);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Error: Invalid Options"));

		/* initialize for the data transfer */
		return_value = (ai_service->init_service_data) (NULL, ai_service, NULL);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot make options"));
		(ai_service->rest_transfer) (ai_service);

		/*
		 * ereport(INFO, (errmsg("%s
		 * ",(char*)(ai_service->rest_response->data))));
		 */
		sprintf(query_string, "%s", (char *) (ai_service->service_data->response));
		/* sprintf(query_string, "%s", "Select * from movies where id < 10"); */
		/* ereport(INFO,(errmsg("QUERY_STRING :%s\n",query_string))); */
		/* --------------------------------------------------------------------- */
		query_data = palloc(sizeof(srf_query_data));
		query_data->current_row = 0;
		query_data->print_header_info = true;
		funcctx->user_fctx = query_data;

		/* connect to the server and retrive the matching data */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT)
		{
			ereport(ERROR,
					(errcode(ERRCODE_CONNECTION_FAILURE),
					 errmsg("Cnnection failed with error code %d", spi_result)));
		}
		spi_result = SPI_execute(query_string, true /* read only */ , 0 /* all tuples */ );
		if (spi_result != SPI_OK_SELECT)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("Execution failed with error code %d", spi_result)));
		}

		query_data->max_rows = SPI_processed;
		query_data->tuble_table = SPI_tuptable;
		funcctx->tuple_desc = BlessTupleDesc(SPI_tuptable->tupdesc);

		make_pk_col_name(pk_col, COLUMN_NAME_LEN, get_option_value(ai_service->service_data->options, OPTION_STORE_NAME));
		query_data->tuble_table = remove_columns_from_spitb(query_data->tuble_table, &(funcctx->tuple_desc), (char **) hide_cols, hide_col_count);
		MemoryContextSwitchTo(oldcontext);
	}
	funcctx = SRF_PERCALL_SETUP();
	query_data = funcctx->user_fctx;

	/* print a header TODO: add relation name to the col name */
	if (query_data->print_header_info)
	{
		query_data->print_header_info = false;
		col_count = funcctx->tuple_desc->natts;
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		header = makeStringInfo();
		appendStringInfo(header, "METADATA (");
		for (i = 0; i < col_count; i++)
		{
			Form_pg_attribute attr = TupleDescAttr(funcctx->tuple_desc, i);

			appendStringInfo(header, " %s ", NameStr(attr->attname));
		}
		appendStringInfo(header, ")");
		MemoryContextSwitchTo(oldcontext);
		ereport(INFO, errmsg("%s", header->data));
	}

	/* As long as there is data to be returned */
	if (query_data->current_row < query_data->max_rows)
	{
		tuple = query_data->tuble_table->vals[query_data->current_row];
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		result = heap_copy_tuple_as_datum(tuple, funcctx->tuple_desc);
		MemoryContextSwitchTo(oldcontext);
		query_data->current_row++;
		SRF_RETURN_NEXT(funcctx, result);
	}
	else
	{
		/* ereport(INFO,(errmsg("DONE returning rows \n"))); */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		SPI_finish();
		MemoryContextSwitchTo(oldcontext);
		SRF_RETURN_DONE(funcctx);
	}

	/* unreachable */
	PG_RETURN_VOID();
}
