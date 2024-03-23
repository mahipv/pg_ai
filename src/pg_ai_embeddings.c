#include <postgres.h>
#include <funcapi.h>

#include "ai_service.h"
#include "utils_pg_ai.h"
#include "guc/pg_ai_guc.h"
#include "rest/rest_transfer.h"

/*
 * The implementation of SQL FUNCTION create_vector_store.
 */
PG_FUNCTION_INFO_V1(pg_ai_create_vector_store);
Datum pg_ai_create_vector_store(PG_FUNCTION_ARGS)
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
	ai_service->function_flags |= FUNCTION_CREATE_VECTOR_STORE;
	return_value =
		initialize_service(SERVICE_OPENAI, MODEL_OPENAI_EMBEDDINGS, ai_service);
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
 * The implementation of SQL FUNCTION query_vector_store.
 */
typedef struct SrfQueryData
{
	int current_row;
	int max_rows;
	bool print_header_info;
	SPITupleTable *tuble_table;
} SrfQueryData;

PG_FUNCTION_INFO_V1(pg_ai_query_vector_store);
Datum pg_ai_query_vector_store(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	MemoryContext oldcontext;
	SrfQueryData *query_data;
	Datum result;
	char *query_string;
	uint32_t spi_result;
	int i;
	int col_count;
	HeapTuple tuple;
	StringInfo header;
	AIService *ai_service;
	int return_value;
	char pk_col[COLUMN_NAME_LEN];
	char *hide_cols[] = {EMBEDDINGS_COLUMN_NAME, OPTION_SIMILARITY_ALGORITHM,
						 pk_col};
	int hide_col_count = sizeof(hide_cols) / sizeof(hide_cols[0]);

	if (SRF_IS_FIRSTCALL())
	{
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		ai_service = palloc0(sizeof(AIService));
		query_string = palloc0(SERVICE_MAX_RESPONSE_SIZE);
		ai_service->memory_context = funcctx->multi_call_memory_ctx;

		ai_service->function_flags |= FUNCTION_QUERY_VECTOR_STORE;
		/* initialize based on the service and model */
		return_value = initialize_service(SERVICE_OPENAI,
										  MODEL_OPENAI_EMBEDDINGS, ai_service);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

		/* pass on the arguments to the service to validate */
		return_value =
			(ai_service->set_and_validate_options)(ai_service, fcinfo);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Error: Invalid Options"));

		/* initialize for the data transfer */
		return_value = (ai_service->set_service_data)(ai_service, NULL);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot make\
											  options"));

		/* prepare for transfer */
		return_value = (ai_service->prepare_for_transfer)(ai_service);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot set \
										 transfer data"));

		/* call the transfer. The rest call will return the query string with
		 * the vectors for the natural language query */
		(ai_service->rest_transfer)(ai_service);

		/* get the query string and execute */
		sprintf(query_string, "%s",
				(char *)(ai_service->service_data->response));

		query_data = palloc(sizeof(SrfQueryData));
		query_data->current_row = 0;
		query_data->print_header_info = true;
		funcctx->user_fctx = query_data;

		/* connect to the server and retrive the matching data */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT)
		{
			ereport(ERROR, (errcode(ERRCODE_CONNECTION_FAILURE),
							errmsg("Cnnection failed with error code %d",
								   spi_result)));
		}
		spi_result =
			SPI_execute(query_string, true /* read only */, 0 /* all tuples */);

		if (spi_result != SPI_OK_SELECT)
		{
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("Execution failed with error code %d",
								   spi_result)));
		}

		query_data->max_rows = SPI_processed;
		query_data->tuble_table = SPI_tuptable;
		funcctx->tuple_desc = BlessTupleDesc(SPI_tuptable->tupdesc);

		make_pk_col_name(pk_col, COLUMN_NAME_LEN,
						 get_option_value(ai_service->service_data->options,
										  OPTION_STORE_NAME));
		query_data->tuble_table = remove_columns_from_spitb(
			query_data->tuble_table, &(funcctx->tuple_desc), (char **)hide_cols,
			hide_col_count);
		MemoryContextSwitchTo(oldcontext);
	} /* end of first call */

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
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		SPI_finish();
		MemoryContextSwitchTo(oldcontext);
		SRF_RETURN_DONE(funcctx);
	}

	/* unreachable */
	PG_RETURN_VOID();
}
