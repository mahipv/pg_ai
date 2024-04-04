#include "service_embeddings.h"

#include "executor/spi.h"

#include "core/utils_pg_ai.h"
#include "guc/pg_ai_guc.h"
#include "rest/rest_transfer.h"

/*
 * Function to define aptions applicable to the embeddings service calls.
 * These options are to be passed via SQL function calls or set by GUCs.
 */
static void define_options(AIService *ai_service)
{
	ServiceOption **option_list = &(ai_service->service_data->options);

	/* define the common options from the base */
	ai_service->define_common_options(ai_service);

	/* the vector store to be craeted or queried */
	define_new_option(option_list, OPTION_STORE_NAME, OPTION_STORE_NAME_DESC,
					  OPTION_FLAG_REQUIRED | OPTION_FLAG_HELP_DISPLAY,
					  NULL /* storage ptr */, 0 /* max size */);

	/*
	 * vector store creation options: 1.SQL query 2.optional notes on the
	 * resultant data set that can be used by LLM to interpret the  col values
	 */
	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		/* the query is text and passed to SPI for execution */
		define_new_option(option_list, OPTION_SQL_QUERY, OPTION_SQL_QUERY_DESC,
						  OPTION_FLAG_REQUIRED | OPTION_FLAG_HELP_DISPLAY,
						  NULL /* storage ptr */, 0 /* max size */);
		define_new_option(option_list, OPTION_NL_NOTES, OPTION_NL_NOTES_DESC,
						  OPTION_FLAG_REQUIRED | OPTION_FLAG_HELP_DISPLAY,
						  NULL /* storage ptr */, 0 /* max size */);
	}

	/*
	 * vector store query options: 1.NL query 2. record count 3. matching
	 * algorithm
	 */
	if (ai_service->function_flags & FUNCTION_QUERY_VECTOR_STORE)
	{
		define_new_option(option_list, OPTION_NL_QUERY, OPTION_NL_QUERY_DESC,
						  OPTION_FLAG_REQUIRED | OPTION_FLAG_HELP_DISPLAY,
						  NULL /* storage ptr */, 0 /* max size */);
		define_new_option(option_list, OPTION_RECORD_COUNT,
						  OPTION_RECORD_COUNT_DESC,
						  OPTION_FLAG_REQUIRED | OPTION_FLAG_HELP_DISPLAY,
						  NULL /* storage ptr */, 0 /* max size */);
		define_new_option(option_list, OPTION_SIMILARITY_ALGORITHM,
						  OPTION_SIMILARITY_ALGORITHM_DESC,
						  OPTION_FLAG_REQUIRED, NULL /* storage ptr */,
						  0 /* max size */);
	}
}

/*
 * Initialize the service options for the embeddings service.
 */
void embeddings_initialize_service(void *service)
{
	AIService *ai_service = (AIService *)service;

	/* define the options for this service - stored in service data */
	define_options(ai_service);
}

/*
 * Return the help text to be displayed for the embeddings service.
 */
void embeddings_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, EMBEDDINGS_HELP, max_len);
}

/*
 * set the options for the embeddings service and validate the options.
 */
int embeddings_set_and_validate_options(void *service, void *function_options)
{
	PG_FUNCTION_ARGS = (FunctionCallInfo)function_options;
	AIService *ai_service = (AIService *)service;
	ServiceOption *options = ai_service->service_data->options;
	char count_str[10];
	int count;

	if (!PG_ARGISNULL(0))
		set_option_value(options, OPTION_STORE_NAME,
						 NameStr(*PG_GETARG_NAME(0)), false /* concat */);

	/* function: create store */
	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		if (!PG_ARGISNULL(1))
			set_option_value(options, OPTION_SQL_QUERY,
							 text_to_cstring(PG_GETARG_TEXT_PP(1)),
							 false /* concat */);

		if (!PG_ARGISNULL(2))
			set_option_value(options, OPTION_NL_NOTES,
							 NameStr(*PG_GETARG_NAME(2)), false /* concat */);
	}

	/* function: query store */
	if (ai_service->function_flags & FUNCTION_QUERY_VECTOR_STORE)
	{
		if (!PG_ARGISNULL(1))
			set_option_value(options, OPTION_NL_QUERY,
							 text_to_cstring(PG_GETARG_TEXT_PP(1)),
							 false /* concat */);

		/* limiting the record count out to MAX_COUNT_RECORDS */
		count = PG_ARGISNULL(2) ? MIN_COUNT_RECORDS : PG_GETARG_INT16((2));
		if (count > MAX_COUNT_RECORDS)
			count = MAX_COUNT_RECORDS;
		if (count < 0)
			count = MIN_COUNT_RECORDS;
		sprintf(count_str, "%d", count);
		set_option_value(options, OPTION_RECORD_COUNT, count_str,
						 false /* concat */);

		set_similarity_algorithm(options);
	}

	/* check if all required options are set */
	for (options = ai_service->service_data->options; options;
		 options = options->next)
	{
		if ((options->flags & OPTION_FLAG_REQUIRED) &&
			!(options->flags & OPTION_FLAG_IS_SET))
		{
			ereport(INFO, (errmsg("Required %s option \"%s\" missing.\n",
								  options->flags & OPTION_FLAG_GUC ? "GUC" :
																	 "function",
								  options->name)));
			return RETURN_ERROR;
		}
	}
	return RETURN_ZERO;
}

/*
 * Function to initialize the service data for the embeddings service.
 */
int embeddings_set_service_data(void *service, void *data)
{
	/* embeddings use pg_vector extension */
	if (!is_extension_installed(PG_EXTENSION_PG_VECTOR))
	{
		ereport(INFO, (errmsg("Extension '%s' is not installed.\n",
							  PG_EXTENSION_PG_VECTOR)));
		return RETURN_ERROR;
	}

	return RETURN_ZERO;
}

/*
 * Function to prepare the service for data transfer. This function is
 * called before the transfer is initiated. Create the table to store the
 * embeddings if the function is to create a vector store.
 */
int embeddings_prepare_for_transfer(void *service)
{
	AIService *ai_service = (AIService *)service;

	/* TODO check if this has/can to be moved to embeddings_set_data() above */
	/* create the data store table and add PK and the embeddings columns */
	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		char query[SQL_QUERY_MAX_LENGTH];

		/* create a table to materialize the query result set */
		snprintf(query, SQL_QUERY_MAX_LENGTH, "CREATE TABLE %s AS %s",
				 get_option_value(ai_service->service_data->options,
								  OPTION_STORE_NAME),
				 get_option_value(ai_service->service_data->options,
								  OPTION_SQL_QUERY));
		execute_query_spi(query, false /* read only */);

		/* add a serial column to identify the rows, needed for a update row */
		snprintf(query, SQL_QUERY_MAX_LENGTH,
				 "ALTER TABLE %s ADD COLUMN %s%s SERIAL",
				 get_option_value(ai_service->service_data->options,
								  OPTION_STORE_NAME),
				 get_option_value(ai_service->service_data->options,
								  OPTION_STORE_NAME),
				 PK_SUFFIX);
		execute_query_spi(query, false /* read only */);

		/* add a vector column to store the embeddings */
		snprintf(query, SQL_QUERY_MAX_LENGTH,
				 "ALTER TABLE %s ADD COLUMN %s vector(%d) ",
				 get_option_value(ai_service->service_data->options,
								  OPTION_STORE_NAME),
				 EMBEDDINGS_COLUMN_NAME, EMBEDDINGS_LIST_SIZE);
		execute_query_spi(query, false /* read only */);
	}
	init_rest_transfer((AIService *)ai_service);
	return RETURN_ZERO;
}

/*
 * Function to cleanup the transfer structures before initiating a new
 * transfer request.
 */
int embeddings_cleanup_service_data(void *ai_service)
{
	/* cleanup_rest_transfer((AIService *)ai_service); */
	pfree(((AIService *)ai_service)->service_data);
	return RETURN_ZERO;
}

/*
 * Function to initialize the service buffers for data tranasfer.
 */
void embeddings_set_service_buffers(RestRequest *rest_request,
									RestResponse *rest_response,
									ServiceData *service_data)
{
	rest_request->data = service_data->request_data;
	rest_request->max_size = service_data->max_request_size;

	rest_response->data = service_data->response_data;
	rest_response->max_size = service_data->max_response_size;
}

/*
 * Initialize the service headers in the curl context. This is a
 * call back from REST transfer layer. The curl headers are
 * constructed from this list.
 */
int embeddings_add_rest_headers(CURL *curl, struct curl_slist **headers,
								void *service)
{
	AIService *ai_service = (AIService *)service;
	struct curl_slist *curl_headers = *headers;
	char key_header[128];

	curl_headers =
		curl_slist_append(curl_headers, "Content-Type: application/json");
	snprintf(key_header, sizeof(key_header), "Authorization: Bearer %s",
			 get_option_value(ai_service->service_data->options,
							  OPTION_SERVICE_API_KEY));
	curl_headers = curl_slist_append(curl_headers, key_header);
	*headers = curl_headers;

	return RETURN_ZERO;
}

/*
 * Callback to make the POST header for the REST transfer.
 */
#define EMBEDDINGS_PREFIX "{\"input\":"
#define EMBEDDINGS_MODEL ",\"model\":\"" MODEL_OPENAI_EMBEDDINGS_NAME "\"}"
void embeddings_add_rest_data(char *buffer, const size_t maxlen,
							  const char *data, const size_t len)
{
	strcpy(buffer, EMBEDDINGS_PREFIX);
	strcat(buffer, "\"");
	strcat(buffer, data);
	strcat(buffer, "\"");
	strcat(buffer, EMBEDDINGS_MODEL);
	/* ereport(INFO,(errmsg("POST: %s\n\n", buffer))); */
}

int embeddings_handle_response_headers(void *service, void *user_data)
{
	return RETURN_ZERO;
}

/*
 * Function to make a string out of the column name value pairs to get the
 * corresponding embeddings.
 */
static void add_cols_name_value_to_prompt(char *buf, const size_t max_buf_size,
										  TupleDesc tupdesc, HeapTuple tuple,
										  char *pk_col, int64 *pk_col_value)
{
	bool isnull;
	/* concatinate all column vals to get an embedding */
	for (int i = 1; i <= tupdesc->natts; i++)
	{
		/* skip sending the embedded column itself */
		if (strcmp(SPI_fname(tupdesc, i), EMBEDDINGS_COLUMN_NAME) == 0)
			continue;

		/* added pk column is a row identifier, skip it */
		if (strcmp(SPI_fname(tupdesc, i), pk_col) == 0)
		{
			*pk_col_value =
				DatumGetInt64(SPI_getbinval(tuple, tupdesc, i, &isnull));
			continue;
		}

		/* add the column name and value to the buffer */
		snprintf(buf + strlen(buf), max_buf_size - strlen(buf), " %s:\"%s\"",
				 SPI_fname(tupdesc, i), SPI_getvalue(tuple, tupdesc, i));
	}
}

/*
 * Extract the vector from the json response returned by the service.
 */
#define RESPONSE_JSON_DATA "data"
#define RESPONSE_JSON_EMBEDDING "embedding"
static Datum extract_vector_from_json(char *json)
{
	Datum datas;
	Datum first_choice;
	Datum return_text;

	datas = DirectFunctionCall2(
		json_object_field_text, CStringGetTextDatum(json),
		PointerGetDatum(cstring_to_text(RESPONSE_JSON_DATA)));
	first_choice =
		DirectFunctionCall2(json_array_element_text, datas, PointerGetDatum(0));
	return_text = DirectFunctionCall2(
		json_object_field_text, first_choice,
		PointerGetDatum(cstring_to_text(RESPONSE_JSON_EMBEDDING)));

	return return_text;
}

/*
 * Function to create the embeddings for the data set. The embeddings are
 * stored in the vector store table.
 */
static void create_embeddings(AIService *ai_service, char *query)
{
	int ret;
	int count = 0;
	Datum return_text;
	int64 pk_col_value = -1;
	char pk_col[COLUMN_NAME_LEN];
	char data[SQL_QUERY_MAX_LENGTH];
	char prompt_str[MAX_BYTE_VALUE];
	ServiceOption *options = ai_service->service_data->options;

	/* the start of the prompt is the notes passed to pg_ai_function */
	snprintf(prompt_str, MAX_BYTE_VALUE, "%s",
			 get_option_value(options, OPTION_NL_NOTES));

	/* PK col is for internal reference and not be used for vector */
	make_pk_col_name(pk_col, COLUMN_NAME_LEN,
					 get_option_value(options, OPTION_STORE_NAME));

	/* execute the query to get the data set */
	SPI_connect();
	ret = SPI_exec(query, count);
	if (ret > 0 && SPI_tuptable != NULL)
	{
		size_t max_buf_size = ai_service->service_data->max_request_size;
		char *buf =
			MemoryContextAlloc(ai_service->memory_context, max_buf_size);
		SPITupleTable *tuptable = SPI_tuptable;
		TupleDesc tupdesc = tuptable->tupdesc;

		/* for each row: get embeddings from LLM and update embeddings col */
		for (uint64 j = 0; j < tuptable->numvals; j++)
		{
			/* for each row get the column names:value pairs */
			HeapTuple tuple = tuptable->vals[j];

			/* start the prompt with the NL notes passed to function */
			snprintf(buf, MAX_BYTE_VALUE, "%s ", prompt_str);

			/* concat column name:value pairs to the prompt */
			add_cols_name_value_to_prompt(buf, max_buf_size, tupdesc, tuple,
										  pk_col, &pk_col_value);

			if (DEBUG_LEVEL(PG_AI_DEBUG_3))
				ereport(INFO, (errmsg("PROMPT: %s\n\n", buf)));

			/* add the col values buffer to the request */
			strcpy(ai_service->service_data->request_data, buf);

			/* make a rest call to get the embeddings */
			rest_transfer(ai_service);
			*((char *)(ai_service->rest_response->data) +
			  ai_service->rest_response->data_size) = '\0';

			/* extract the embeddings from the response json */
			if (ai_service->rest_response->response_code == HTTP_OK)
			{
				return_text = extract_vector_from_json(
					(char *)(ai_service->rest_response->data));
				strcpy(data, text_to_cstring(DatumGetTextPP(return_text)));
				remove_new_lines(data);

				/* update the row in the vector store with the embeddings */
				sprintf(query, "UPDATE %s SET %s = '%s' WHERE %s%s = %ld",
						get_option_value(options, OPTION_STORE_NAME),
						EMBEDDINGS_COLUMN_NAME, data,
						get_option_value(options, OPTION_STORE_NAME), PK_SUFFIX,
						pk_col_value);
				execute_query_spi(query, false /* read only */);

				/* make way for the next call */
				ai_service->rest_response->data_size = 0;
				*((char *)(ai_service->rest_response->data) +
				  ai_service->rest_response->data_size) = '\0';
			}
		} /* end of while cursor */
		if (buf)
			pfree(buf);
	} /* rows returned */
	SPI_finish();

	/* return the store name if no error */
	if (ai_service->rest_response->response_code == HTTP_OK)
		strcpy((char *)(ai_service->rest_response->data),
			   get_option_value(options, OPTION_STORE_NAME));
	else if (ai_service->rest_response->data_size == 0)
	{
		/* if no error text from LLM, return stock error string and length */
		strcpy(ai_service->service_data->response_data,
			   GET_ERR_STR(TRANSFER_FAIL));
		ai_service->rest_response->data_size =
			strlen(GET_ERR_STR(TRANSFER_FAIL));
	}
}

/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 */
void embeddings_rest_transfer(void *service)
{
	Datum return_text;
	AIService *ai_service = (AIService *)(service);
	ServiceOption *options = ai_service->service_data->options;
	char data[SQL_QUERY_MAX_LENGTH];
	char query[SQL_QUERY_MAX_LENGTH];

	ai_service = (AIService *)(service);

	/* update the vector store(table) with embeddings */
	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		sprintf(query, "SELECT * FROM %s",
				get_option_value(ai_service->service_data->options,
								 OPTION_STORE_NAME));
		return create_embeddings(ai_service, query);
	}

	/* return a SQL query for the SRF */
	if (ai_service->function_flags & FUNCTION_QUERY_VECTOR_STORE)
	{
		/* get the embeddings for the NL query */
		strcpy(ai_service->service_data->request_data,
			   get_option_value(options, OPTION_NL_QUERY));

		if (DEBUG_LEVEL(PG_AI_DEBUG_3))
			ereport(INFO, (errmsg("PROMPT: %s\n\n",
								  ai_service->service_data->request_data)));
		rest_transfer(ai_service);
		*((char *)(ai_service->rest_response->data) +
		  ai_service->rest_response->data_size) = '\0';

		/* make the SQL query */
		if (ai_service->rest_response->response_code == HTTP_OK)
		{
			/* extract the embeddings from response */
			return_text = extract_vector_from_json(
				(char *)(ai_service->rest_response->data));
			strcpy(data, text_to_cstring(DatumGetTextPP(return_text)));
			remove_new_lines(data);

			/* names in select, to match hide_cols[] in process_result_set() */
			make_embeddings_query(
				query, SQL_QUERY_MAX_LENGTH, data,
				get_option_value(options, OPTION_STORE_NAME),
				EMBEDDINGS_COLUMN_NAME,
				get_option_value(options, OPTION_SIMILARITY_ALGORITHM),
				OPTION_SIMILARITY_ALGORITHM);

			/* add the limit value to the query */
			if (get_option_value(options, OPTION_RECORD_COUNT))
			{
				strcat(query, "LIMIT ");
				strcat(query, get_option_value(options, OPTION_RECORD_COUNT));
			}

			if (DEBUG_LEVEL(PG_AI_DEBUG_3))
				ereport(INFO, (errmsg("QUERY: %s \n\n", query)));

			/* Return SQL: result set is formatted by caller for display */
			strcpy(ai_service->service_data->response_data, query);
		} /* http response ok */
		else if (ai_service->rest_response->data_size == 0)
		{
			/* if no error text from LLM, return stock error string */
			strcpy(ai_service->service_data->response_data,
				   GET_ERR_STR(TRANSFER_FAIL));
			ai_service->rest_response->data_size =
				strlen(GET_ERR_STR(TRANSFER_FAIL));
		}
	} /* query embeddings */
	return;
}

/* this has to be based on the context lengths of the supported services */
void embeddings_get_max_request_response_sizes(size_t *max_request_size,
											   size_t *max_response_size)
{
	*max_request_size = SERVICE_MAX_REQUEST_SIZE;
	*max_response_size = SERVICE_MAX_RESPONSE_SIZE;
}