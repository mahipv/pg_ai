#include <postgres.h>
#include <funcapi.h>

#include "ai_service.h"
#include "utils_pg_ai.h"
#include "guc/pg_ai_guc.h"
#include "rest/rest_transfer.h"

/* struct to maintain state between SRF calls */
typedef struct SrfQueryData
{
	int current_row;
	int max_rows;
	/* result set from the vector store */
	SPITupleTable *tuble_table;
} SrfQueryData;

/*
 * Helper: Given a tuple descriptor, print the column names.
 */
static void print_meta_data(TupleDesc tupdesc)
{
	int i;
	StringInfo header = makeStringInfo();
	appendStringInfo(header, "METADATA (");
	for (i = 0; i < tupdesc->natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
		appendStringInfo(header, " %s ", NameStr(attr->attname));
	}
	appendStringInfo(header, ")");
	ereport(INFO, errmsg("%s", header->data));
}

/*
 * Helper: After the SPI_execution of a query, process the result set.
 * Hides the columns of the result set that are not required for the user.
 */
static void process_result_set(AIService *ai_service, FuncCallContext *funcctx)
{
	char pk_col[COLUMN_NAME_LEN];
	char *hide_cols[] = {EMBEDDINGS_COLUMN_NAME, OPTION_SIMILARITY_ALGORITHM,
						 pk_col};
	int hide_col_count = sizeof(hide_cols) / sizeof(hide_cols[0]);
	SrfQueryData *query_data;

	/* structure to maintain the call state for the SRF */
	query_data = palloc(sizeof(SrfQueryData));
	query_data->current_row = 0;
	query_data->max_rows = SPI_processed;
	query_data->tuble_table = SPI_tuptable;
	funcctx->user_fctx = query_data;

	funcctx->tuple_desc = BlessTupleDesc(SPI_tuptable->tupdesc);

	/* hide the columns that are not required like PK, vector... */
	make_pk_col_name(
		pk_col, COLUMN_NAME_LEN,
		get_option_value(ai_service->service_data->options, OPTION_STORE_NAME));
	query_data->tuble_table = remove_columns_from_spitb(
		query_data->tuble_table, &(funcctx->tuple_desc), (char **)hide_cols,
		hide_col_count);
}

/*
 * The implementation of SQL FUNCTION create_vector_store.
 */
PG_FUNCTION_INFO_V1(pg_ai_create_vector_store);
Datum pg_ai_create_vector_store(PG_FUNCTION_ARGS)
{
	AIService *ai_service = palloc0(sizeof(AIService));
	MemoryContext func_context;
	MemoryContext old_context;
	text *return_text;

	/* check for the column to be interpreted */
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_TEXT_P(GET_ERR_TEXT(ARG_NULL));

	/* Create a new memory context for this PgAi function */
	func_context = AllocSetContextCreate(CurrentMemoryContext, PG_AI_MCTX,
										 ALLOCSET_DEFAULT_SIZES);
	old_context = MemoryContextSwitchTo(func_context);
	ai_service->memory_context = func_context;

	/* set the function specific flag */
	ai_service->function_flags |= FUNCTION_CREATE_VECTOR_STORE;
	INITIALIZE_SERVICE(SERVICE_OPENAI, MODEL_OPENAI_EMBEDDINGS, ai_service);

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
 * The implementation of SQL FUNCTION query_vector_store.
 */
PG_FUNCTION_INFO_V1(pg_ai_query_vector_store);
Datum pg_ai_query_vector_store(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	MemoryContext oldcontext;
	SrfQueryData *query_data;
	Datum result;
	char *query_string;
	uint32_t spi_result;
	HeapTuple tuple;
	AIService *ai_service;

	if (SRF_IS_FIRSTCALL())
	{
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		ai_service = palloc0(sizeof(AIService));
		query_string = palloc0(SERVICE_MAX_RESPONSE_SIZE);
		ai_service->memory_context = funcctx->multi_call_memory_ctx;

		/* initialize based on the service and model */
		ai_service->function_flags |= FUNCTION_QUERY_VECTOR_STORE;
		INITIALIZE_SERVICE(SERVICE_OPENAI, MODEL_OPENAI_EMBEDDINGS, ai_service);

		/* pass on the arguments to the service to validate */
		SET_AND_VALIDATE_OPTIONS(ai_service, fcinfo);

		/* initialize for the data transfer */
		SET_SERVICE_DATA(ai_service, NULL);

		/* prepare for transfer */
		PREPARE_FOR_TRANSFER(ai_service);

		/* call the transfer. The rest call will return the query string with
		 * the vectors for the natural language query */
		REST_TRANSFER(ai_service);

		/* get the query string and execute */
		sprintf(query_string, "%s",
				(char *)(ai_service->service_data->response));

		/* connect to the server and retrive the matching data */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT)
		{
			ereport(ERROR, (errcode(ERRCODE_CONNECTION_FAILURE),
							errmsg("Cnnection failed with error code %d",
								   spi_result)));
		}

		/* execute the query */
		spi_result =
			SPI_execute(query_string, true /* read only */, 0 /* all tuples */);

		if (spi_result != SPI_OK_SELECT)
		{
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("Execution failed with error code %d",
								   spi_result)));
		}

		/* process the resulset */
		process_result_set(ai_service, funcctx);
		print_meta_data(funcctx->tuple_desc);
		MemoryContextSwitchTo(oldcontext);
	} /* end of first call */

	/*
	 * This section is executed for all the cols of the SRF including the first
	 * call
	 */
	funcctx = SRF_PERCALL_SETUP();
	query_data = funcctx->user_fctx;

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
