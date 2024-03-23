#include "service_embeddings.h"

#include "executor/spi.h"

#include "rest/rest_transfer.h"
#include "guc/pg_ai_guc.h"
#include "utils_pg_ai.h"

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

	/* TODO : change func signature to return error in mem context is not set */
	/* the options are stored in service data, allocate before defining */
	ai_service->service_data =
		MemoryContextAllocZero(ai_service->memory_context, sizeof(ServiceData));
	ai_service->service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	ai_service->service_data->max_response_size = SERVICE_MAX_RESPONSE_SIZE;

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
 * Set the similarity algorithm to be used for the embeddings service.
 * The algorithm is set by the GUC pg_ai.vec_similarity_algo and defaults
 * to cosine similarity.
 */
void set_similarity_algorithm(ServiceOption *options)
{
	char *similarity_algo =
		get_pg_ai_guc_string_variable(PG_AI_GUC_VEC_SIMILARITY_ALGO);

	/* default to cosine */
	if (!similarity_algo)
		similarity_algo = EMBEDDINGS_SIMILARITY_COSINE;
	else
	{
		/* check for non-exact strings and set values accordingly */
		if (!strcasecmp(similarity_algo, EMBEDDINGS_SIMILARITY_EUCLIDEAN))
			similarity_algo = EMBEDDINGS_SIMILARITY_EUCLIDEAN;
		else if (!strcasecmp(similarity_algo,
							 EMBEDDINGS_SIMILARITY_INNER_PRODUCT))
			similarity_algo = EMBEDDINGS_SIMILARITY_INNER_PRODUCT;
		else
			similarity_algo = EMBEDDINGS_SIMILARITY_COSINE;
	}

	set_option_value(options, OPTION_SIMILARITY_ALGORITHM, similarity_algo,
					 false /* concat */);
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
 * wrapper to execute the query using SPI
 */
int execute_query_spi(const char *query, bool read_only)
{
	int ret;
	int row_limit = 0;

	/* ereport(INFO,(errmsg("Query: %s\n", query))); */
	SPI_connect();
	ret = SPI_execute(query, read_only, row_limit);
	SPI_finish();
	return ret;
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
	rest_request->data = service_data->request;
	rest_request->max_size = service_data->max_request_size;

	rest_response->data = service_data->response;
	rest_response->max_size = service_data->max_response_size;
}

/*
 * Initialize the service headers in the curl context. This is a
 * call back from REST transfer layer. The curl headers are
 * constructed from this list.
 */
int embeddings_add_service_headers(CURL *curl, struct curl_slist **headers,
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
#define EMBEDDINGS_MODEL ",\"model\":\"text-embedding-ada-002\"}"
void embeddings_post_header_maker(char *buffer, const size_t maxlen,
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
 * Function that prepares the select query for the embeddings service.
 * The query is based on the similarity algorithm set by the user.
 */
static void make_select_query_with_similarity(AIService *ai_service, char *data,
											  char *query)
{
	ServiceOption *options = ai_service->service_data->options;
	char *similarity_algo =
		get_option_value(options, OPTION_SIMILARITY_ALGORITHM);

	/* at this point by this time similarity_algo is set and cannot be NULL */

	/* refer pgvector docs for cosine syntax */
	if (!strcasecmp(similarity_algo, EMBEDDINGS_SIMILARITY_COSINE))
	{
		sprintf(query,
				"SELECT *, 1 - (%s <=> '%s') AS %s FROM %s ORDER BY %s DESC ",
				EMBEDDINGS_COLUMN_NAME, data, OPTION_SIMILARITY_ALGORITHM,
				get_option_value(options, OPTION_STORE_NAME),
				OPTION_SIMILARITY_ALGORITHM);
	}

	/* refer pgvector docs for euclidean distance syntax */
	if (!strcasecmp(similarity_algo, EMBEDDINGS_SIMILARITY_EUCLIDEAN))
	{
		sprintf(query, "SELECT *, (%s <-> '%s') AS %s FROM %s ORDER BY %s ASC ",
				EMBEDDINGS_COLUMN_NAME, data, OPTION_SIMILARITY_ALGORITHM,
				get_option_value(options, OPTION_STORE_NAME),
				OPTION_SIMILARITY_ALGORITHM);
	}

	/* refer pgvector docs for the inner product syntax */
	if (!strcasecmp(similarity_algo, EMBEDDINGS_SIMILARITY_INNER_PRODUCT))
	{
		sprintf(
			query,
			"SELECT *, (-1 * (%s <#> '%s')) AS %s FROM %s ORDER BY %s DESC ",
			EMBEDDINGS_COLUMN_NAME, data, OPTION_SIMILARITY_ALGORITHM,
			get_option_value(options, OPTION_STORE_NAME),
			OPTION_SIMILARITY_ALGORITHM);
	}
}

/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 */
/* TODO break the function into mangeable chunks */
void embeddings_rest_transfer(void *service)
{
	Datum datas;
	Datum first_choice;
	Datum return_text;
	Datum val;
	bool isnull;
	int ret;
	int count = 0;
	int64 pk_col_value = -1;
	AIService *ai_service = (AIService *)(service);
	ServiceOption *options = ai_service->service_data->options;
	char pk_col[COLUMN_NAME_LEN];
	char data[SQL_QUERY_MAX_LENGTH];
	char query[SQL_QUERY_MAX_LENGTH];

	ai_service = (AIService *)(service);
	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		sprintf(query, "SELECT * FROM %s",
				get_option_value(ai_service->service_data->options,
								 OPTION_STORE_NAME));

		/*
		 * ereport(INFO, (errmsg("\nProcessing query \"%s\"\n",
		 * get_option_value(ai_service->service_data->options,
		 * OPTION_SQL_QUERY))));
		 */
		SPI_connect();
		ret = SPI_exec(query, count);

		/*
		 * for each fetched row, get the embeddings from the service and
		 * update * the row in the materialized table to set the embeddings.
		 */
		if (ret > 0 && SPI_tuptable != NULL)
		{
			SPITupleTable *tuptable = SPI_tuptable;
			TupleDesc tupdesc = tuptable->tupdesc;
			char buf[8192];
			uint64 j;
			char prompt_str[64];

			snprintf(prompt_str, 64, "%s",
					 get_option_value(options, OPTION_NL_NOTES));
			make_pk_col_name(pk_col, COLUMN_NAME_LEN,
							 get_option_value(options, OPTION_STORE_NAME));
			snprintf(buf, 8192, "%s ", prompt_str);

			/* loop through the result set */
			for (j = 0; j < tuptable->numvals; j++)
			{
				HeapTuple tuple = tuptable->vals[j];
				int i;

				/* concatinate all column vals to get an embedding */
				for (i = 1, buf[0] = 0; i <= tupdesc->natts; i++)
				{
					/* skip sending the embedded column itself */
					if (strcmp(SPI_fname(tupdesc, i), EMBEDDINGS_COLUMN_NAME) ==
						0)
						continue;

					/* newly added pk column is a row identifier, skip it */
					if (strcmp(SPI_fname(tupdesc, i), pk_col) == 0)
					{
						val = SPI_getbinval(tuple, tupdesc, i, &isnull);
						pk_col_value = DatumGetInt64(val);
						continue;
					}

					/* add the column name and value to the buffer */
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
							 " %s:\"%s\"", SPI_fname(tupdesc, i),
							 SPI_getvalue(tuple, tupdesc, i));
				}

				/* add the col values buffer to the request */
				strcpy(ai_service->service_data->request, buf);
				/* ereport(INFO,(errmsg("Buffer: %s\n\n", buf))); */

				/* make a rest call to get the embeddings */
				rest_transfer(ai_service);
				*((char *)(ai_service->rest_response->data) +
				  ai_service->rest_response->data_size) = '\0';

				/*
				 * ereport(INFO,(errmsg("Return: %s -%ld\n\n",
				 * (char*)(ai_service->rest_response->data), pk_col_value)));
				 */

				/* extract the embeddings from the response json */
				if (ai_service->rest_response->response_code == HTTP_OK)
				{
					datas = DirectFunctionCall2(
						json_object_field_text,
						CStringGetTextDatum(
							(char *)(ai_service->rest_response->data)),
						PointerGetDatum(cstring_to_text("data")));
					first_choice = DirectFunctionCall2(
						json_array_element_text, datas, PointerGetDatum(0));
					return_text = DirectFunctionCall2(
						json_object_field_text, first_choice,
						PointerGetDatum(cstring_to_text("embedding")));

					/*
					 * ereport(INFO,(errmsg("Return1: %s \n\n",
					 * text_to_cstring(DatumGetTextPP(return_text)))));
					 */
					strcpy(ai_service->service_data->response,
						   text_to_cstring(DatumGetTextPP(datas)));
					strcpy(data, text_to_cstring(DatumGetTextPP(return_text)));
					remove_new_lines(data);

					/* update the row with the embeddings */
					sprintf(query, "UPDATE %s SET %s = '%s' WHERE %s%s = %ld",
							get_option_value(options, OPTION_STORE_NAME),
							EMBEDDINGS_COLUMN_NAME, data,
							get_option_value(options, OPTION_STORE_NAME),
							PK_SUFFIX, pk_col_value);
					execute_query_spi(query, false /* read only */);

					/* make way for the next call */
					ai_service->rest_response->data_size = 0;
					*((char *)(ai_service->rest_response->data) +
					  ai_service->rest_response->data_size) = '\0';
				}
				else if (ai_service->rest_response->data_size == 0)
				{
					strcpy(ai_service->service_data->response,
						   "Something is not ok, try again.");
					return;
				}

			} /* end of while cursor */
		}	  /* rows returned */
		SPI_finish();
		/* return the store name */
		strcpy((char *)(ai_service->rest_response->data),
			   get_option_value(options, OPTION_STORE_NAME));
	} /* create embeddings */

	/* query embeddings */
	if (ai_service->function_flags & FUNCTION_QUERY_VECTOR_STORE)
	{
		/* get the embeddings for the NL query */
		strcpy(ai_service->service_data->request,
			   get_option_value(options, OPTION_NL_QUERY));
		rest_transfer(ai_service);
		*((char *)(ai_service->rest_response->data) +
		  ai_service->rest_response->data_size) = '\0';

		/*
		 * ereport(INFO,(errmsg("Return: %s\n\n",
		 * ai_service->rest_response->data)));
		 */

		/* extract the embeddings from the response */
		if (ai_service->rest_response->response_code == HTTP_OK)
		{
			datas = DirectFunctionCall2(
				json_object_field_text,
				CStringGetTextDatum((char *)(ai_service->rest_response->data)),
				PointerGetDatum(cstring_to_text("data")));
			first_choice = DirectFunctionCall2(json_array_element_text, datas,
											   PointerGetDatum(0));
			return_text = DirectFunctionCall2(
				json_object_field_text, first_choice,
				PointerGetDatum(cstring_to_text("embedding")));

			strcpy(data, text_to_cstring(DatumGetTextPP(return_text)));
			remove_new_lines(data);

			/* query for the materialized store for ANN */
			make_select_query_with_similarity(ai_service, data, query);

			if (get_option_value(options, OPTION_RECORD_COUNT))
			{
				strcat(query, "LIMIT ");
				strcat(query, get_option_value(options, OPTION_RECORD_COUNT));
			}

			if (is_debug_level(PG_AI_DEBUG_3))
				ereport(INFO, (errmsg("QUERY: %s \n\n", query)));

			/*
			 * Return the SQL query so that the result set can formatted by
			 * the caller for display.
			 */
			strcpy(ai_service->service_data->response, query);
			return;
		} /* http response ok */
	}	  /* query embeddings */
	return;
}
