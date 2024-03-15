#include "service_embeddings.h"
#include <ctype.h>
#include <malloc.h>
#include "executor/spi.h"
#include "utils/builtins.h"
#include "postgres.h"
#include "ai_config.h"
#include "utils_pg_ai.h"
#include "rest/rest_transfer.h"
#include "guc/pg_ai_guc.h"

/*
 * Function to define aptions applicable to chatgpt service. The values for
 * this options are read from the json options file.
 *
 * @param[out]	service_data	options added to the service specific data
 * @return		void
 *
 */
static void
define_options(AIService * ai_service)
{
	ServiceData *service_data = ai_service->service_data;

	define_new_option(&(service_data->options), OPTION_SERVICE_NAME,
					  OPTION_SERVICE_NAME_DESC, 1 /* guc */ , 1 /* required */ , false /* help_display */ );
	define_new_option(&(service_data->options), OPTION_MODEL_NAME,
					  OPTION_MODEL_NAME_DESC, 1 /* guc */ , 1 /* required */ , false /* help_display */ );
	define_new_option(&(service_data->options), OPTION_SERVICE_API_KEY,
					  OPTION_SERVICE_API_KEY_DESC, 1 /* guc_option */ , 1 /* required */ , false /* help_display */ );
	define_new_option(&(service_data->options), OPTION_STORE_NAME,
					  OPTION_STORE_NAME_DESC, 0 /* guc_option */ , 1 /* required */ , true /* help_display */ );

	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		define_new_option(&(service_data->options), OPTION_SQL_QUERY,
						  OPTION_SQL_QUERY_DESC, 0 /* guc_option */ , 1 /* required */ , true /* help_display */ );
		define_new_option(&(service_data->options), OPTION_SERVICE_PROMPT,
						  OPTION_SERVICE_PROMPT_DESC, 0 /* guc_option */ , 0 /* required */ , true /* help_display */ );
	}

	if (ai_service->function_flags & FUNCTION_QUERY_VECTOR_STORE)
	{
		define_new_option(&(service_data->options), OPTION_NL_QUERY,
						  OPTION_NL_QUERY_DESC, 0 /* guc_option */ , 1 /* required */ , true /* help_display */ );
		define_new_option(&(service_data->options), OPTION_RECORD_COUNT,
						  OPTION_RECORD_COUNT_DESC, 0 /* guc_option */ , 1 /* required */ , true /* help_display */ );
		define_new_option(&(service_data->options), OPTION_MATCHING_ALGORITHM,
						  OPTION_MATCHING_ALGORITHM_DESC, 0 /* guc_option */ , 1 /* required */ , false /* help_display */ );
	}
}

/*
 * Initialize the options to be used for chatgpt. The options will hold
 * information about the AI service and some of them will be used in the curl
 * headers for REST transfer.
 *
 * @param[out]	service_data	options added to the service specific data
 * @return		void
 *
 */
void
embeddings_init_service_options(void *service)
{
	AIService  *ai_service = (AIService *) service;
	ServiceData *service_data;

	service_data = (ServiceData *) palloc0(sizeof(ServiceData));
	ai_service->service_data = service_data;

	define_options(ai_service);

	/*
	 * unused options but need to be there for help and when multiple services
	 * and models are supported
	 */
	set_option_value(ai_service->service_data->options, OPTION_SERVICE_NAME, get_service_name(ai_service));
	set_option_value(ai_service->service_data->options, OPTION_MODEL_NAME, get_model_name(ai_service));
}

/*
 * Return the help text to be displayed for the chatgpt service
 *
 * @param[out]	help_text	the help text is reurned in this array
 * @param[in]	max_len		the max length of the help text the array can hold
 * @return		void
 *
 */
void
embeddings_help(char *help_text, const size_t max_len)
{
	if (help_text)
		strncpy(help_text, EMBEDDINGS_HELP, max_len);
}

int
embeddings_set_and_validate_options(void *service, void *function_options)
{
	PG_FUNCTION_ARGS = (FunctionCallInfo) function_options;
	AIService  *ai_service = (AIService *) service;
	ServiceOption *options;
	char		count_str[10];
	int			count;

	if (!PG_ARGISNULL(0))
		set_option_value(ai_service->service_data->options, OPTION_STORE_NAME,
						 NameStr(*PG_GETARG_NAME(0)));

	/* create store function */
	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		if (!PG_ARGISNULL(1))
			set_option_value(ai_service->service_data->options, OPTION_SQL_QUERY,
							 text_to_cstring(PG_GETARG_TEXT_PP(1)));
		if (!PG_ARGISNULL(2))
			set_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT,
							 NameStr(*PG_GETARG_NAME(2)));
	}

	/* query store function */
	if (ai_service->function_flags & FUNCTION_QUERY_VECTOR_STORE)
	{
		if (!PG_ARGISNULL(1))
			set_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT,
							 text_to_cstring(PG_GETARG_TEXT_PP(1)));

		/* limiting the record count out to MAX_COUNT_RECORDS */
		count = PG_ARGISNULL(2) ? MIN_COUNT_RECORDS : PG_GETARG_INT16((2));
		if (count > MAX_COUNT_RECORDS)
			count = MAX_COUNT_RECORDS;
		if (count < 0)
			count = MIN_COUNT_RECORDS;
		sprintf(count_str, "%d", count);
		set_option_value(ai_service->service_data->options, OPTION_RECORD_COUNT, count_str);

		/* TODO for now only consine similarity is supported */
		set_option_value(ai_service->service_data->options, OPTION_MATCHING_ALGORITHM,
						 EMBEDDINGS_COSINE_SIMILARITY);
	}

	if (get_pg_ai_guc_variable(PG_AI_GUC_API_KEY))
		set_option_value(ai_service->service_data->options, OPTION_SERVICE_API_KEY, get_pg_ai_guc_variable(PG_AI_GUC_API_KEY));


	options = ai_service->service_data->options;
	while (options)
	{
		/* paramters passed to function take presidence */
		if (options->is_set)
		{
			options = options->next;
			continue;
		}
		else
		{
			/* required and not set */
			if (options->required)
			{
				ereport(INFO, (errmsg("Required value for option \"%s\" missing.\n", options->name)));
				return RETURN_ERROR;
			}
		}
		options = options->next;
	}
	return RETURN_ZERO;
}

/*
 * This function is called from the PG layer after it has the table data and
 * before invoking the REST transfer call. Load the json options(will be used
 * for the transfer)and copy the data received from PG into the REST request
 * structures.
 *
 * @param[in]		file_path	the help text is reurned in this array
 * @param[in/out]	ai_service	pointer to the AIService which has the
 *								defined option list
 * @return			zero on success, non-zero otherwise
 *
 */
int
embeddings_init_service_data(void *options, void *service, void *file_path)
{
	ServiceData *service_data;
	AIService  *ai_service = (AIService *) service;

	service_data = ai_service->service_data;
	service_data->max_request_size = SERVICE_MAX_REQUEST_SIZE;
	service_data->max_response_size = SERVICE_MAX_RESPONSE_SIZE;

	/* TODO convert all these to the options to be read from the option file */
	/* initialize data partly here */
	strcpy(service_data->url, EMBEDDINGS_API_URL);

	/* embeddings use pg_vector extension */
	if (!is_extension_installed(PG_EXTENSION_PG_VECTOR))
	{
		ereport(INFO, (errmsg("Extension '%s' is not installed.\n", PG_EXTENSION_PG_VECTOR)));
		return RETURN_ERROR;
	}

	/* print_service_options(ai_service->service_data->options, 1); */

	/*
	 * TODO
	 *
	 * 0. check for injections, pg_vector 1. Issue a create table with the SQL
	 * provided + add a PK + add a column 'embeddings' to it 2. Query the
	 * newly created table, for each row data a. make a reasonable string with
	 * column names and values b. REST query the API to get the embeddings c.
	 * update the embeddings column for each row
	 */

	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		char		query[SQL_QUERY_MAX_LENGTH];

		snprintf(query, SQL_QUERY_MAX_LENGTH, "CREATE TABLE %s AS %s",
				 get_option_value(ai_service->service_data->options, OPTION_STORE_NAME),
				 get_option_value(ai_service->service_data->options, OPTION_SQL_QUERY));
		execute_query_spi(query, false /* read only */ );

		snprintf(query, SQL_QUERY_MAX_LENGTH, "ALTER TABLE %s ADD COLUMN %s%s SERIAL",
				 get_option_value(ai_service->service_data->options, OPTION_STORE_NAME),
				 get_option_value(ai_service->service_data->options, OPTION_STORE_NAME),
				 PK_SUFFIX);
		execute_query_spi(query, false /* read only */ );

		snprintf(query, SQL_QUERY_MAX_LENGTH, "ALTER TABLE %s ADD COLUMN %s vector(%d) ",
				 get_option_value(ai_service->service_data->options, OPTION_STORE_NAME),
				 EMBEDDINGS_COLUMN_NAME,
				 EMBEDDINGS_LIST_SIZE);
		execute_query_spi(query, false /* read only */ );
	}
	init_rest_transfer((AIService *) ai_service);
	return RETURN_ZERO;
}

int
execute_query_spi(const char *query, bool read_only)
{
	int			ret;
	int			row_limit = 0;

	/* unlimited */
	/* ereport(INFO,(errmsg("Query: %s\n", query))); */
	SPI_connect();
	ret = SPI_execute(query, read_only, row_limit);
	SPI_finish();
	return ret;
}


/*
 * Function to cleanup the transfer structures before initiating a new
 * transfer request.
 *
 * @param[in/out]	ai_service	pointer to the AIService which has the
 * @return			void
 *
 */
int
embeddings_cleanup_service_data(void *ai_service)
{
	/* cleanup_rest_transfer((AIService *)ai_service); */
	pfree(((AIService *) ai_service)->service_data);
	return RETURN_ZERO;
}

/*
 * Function to initialize the service buffers for data tranasfer.
 *
 * @param[out]	rest_request	pointer to the REST request data
 * @param[out]	rest_reponse	pointer to the REST response data
 * @param[in]	service_data	pointer to the service specific data
 * @return		void
 *
 */
void
embeddings_set_service_buffers(RestRequest * rest_request,
							   RestResponse * rest_response,
							   ServiceData * service_data)
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
 *
 * @param[out]	curl			pointer curl conext
 * @param[out]	curl_slist		add new headers to this list
 * @param[in]	service_data	pointer to AIService
 * @return		zero on success, non-zero otherwise
 *
 */
int
embeddings_add_service_headers(CURL * curl, struct curl_slist **headers,
							   void *service)
{
	AIService  *ai_service = (AIService *) service;
	struct curl_slist *curl_headers = *headers;
	char		key_header[128];

	curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
	snprintf(key_header, sizeof(key_header), "Authorization: Bearer %s",
			 get_option_value(ai_service->service_data->options, OPTION_SERVICE_API_KEY));
	curl_headers = curl_slist_append(curl_headers, key_header);
	*headers = curl_headers;

	return RETURN_ZERO;
}


/* TODO the token count should be dynamic */
/*
 * Callback to make the post header
 *
 * @param[out]	buffer	the post header is returned in this
 * @param[in]	maxlen	the max length the buffer can accomodate
 * @param[in]	data	the data from which post header is created
 * @param[in]	len		length of the data
 * @return		void
 *
 */

#define EMBEDDINGS_PREFIX "{\"input\":"
#define EMBEDDINGS_MODEL ",\"model\":\"text-embedding-ada-002\"}"

void
embeddings_post_header_maker(char *buffer, const size_t maxlen,
							 const char *data, const size_t len)
{
	strcpy(buffer, EMBEDDINGS_PREFIX);
	strcat(buffer, "\"");
	strcat(buffer, data);
	strcat(buffer, "\"");
	strcat(buffer, EMBEDDINGS_MODEL);
	/* ereport(INFO,(errmsg("POST: %s\n\n", buffer))); */
}

void
removeNewlines(char *stream)
{
	char	   *p = stream;

	/* Pointer to iterate over the stream */
	char	   *q = stream;

	/* Pointer to write the modified stream(without newlines) */
	while (*p != '\0')
	{
		if ((*p != '\n') && (*p != ' '))
		{
			*q = *p;
			q++;
		}
		p++;
	}
	*q = '\0';
	/* Add the null terminator to mark the end of the modified stream */
}

int
embeddings_handle_response_headers(void *service, void *user_data)
{
	return RETURN_ZERO;
}

/*
 * Function to initiate the curl transfer and extract the response from
 * the json returned by the service.
 *
 * @param[in]	service		pointer to the service specific data
 * @return		void
 *
 */
void
embeddings_rest_transfer(void *service)
{
	Datum		datas;
	Datum		first_choice;
	Datum		return_text;
	Datum		val;
	bool		isnull;
	AIService  *ai_service;
	int			ret;
	int			count = 0;
	int64		pk_col_value = -1;
	char		pk_col[COLUMN_NAME_LEN];
	char		data[SQL_QUERY_MAX_LENGTH];
	char		query[SQL_QUERY_MAX_LENGTH];

	ai_service = (AIService *) (service);
	if (ai_service->function_flags & FUNCTION_CREATE_VECTOR_STORE)
	{
		sprintf(query, "SELECT * FROM %s", get_option_value(ai_service->service_data->options, OPTION_STORE_NAME));

		/*
		 * ereport(INFO, (errmsg("\nProcessing query \"%s\"\n",
		 * get_option_value(ai_service->service_data->options,
		 * OPTION_SQL_QUERY))));
		 */
		SPI_connect();
		ret = SPI_exec(query, count);
		/* if rows fetched */
		if (ret > 0 && SPI_tuptable != NULL)
		{
			SPITupleTable *tuptable = SPI_tuptable;
			TupleDesc	tupdesc = tuptable->tupdesc;
			char		buf[8192];
			uint64		j;
			char		prompt_str[64];

			snprintf(prompt_str, 64, "%s", get_option_value(ai_service->service_data->options, OPTION_SERVICE_PROMPT));
			make_pk_col_name(pk_col, COLUMN_NAME_LEN, get_option_value(ai_service->service_data->options, OPTION_STORE_NAME));
			snprintf(buf, 8192, "%s ", prompt_str);
			for (j = 0; j < tuptable->numvals; j++)
			{
				HeapTuple	tuple = tuptable->vals[j];
				int			i;

				for (i = 1, buf[0] = 0; i <= tupdesc->natts; i++)
				{
					if (strcmp(SPI_fname(tupdesc, i), EMBEDDINGS_COLUMN_NAME) == 0)
						continue;
					if (strcmp(SPI_fname(tupdesc, i), pk_col) == 0)
					{
						val = SPI_getbinval(tuple, tupdesc, i, &isnull);
						pk_col_value = DatumGetInt64(val);
						continue;
					}
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %s:\"%s\"",
							 SPI_fname(tupdesc, i),
							 SPI_getvalue(tuple, tupdesc, i));
				}
				strcpy(ai_service->service_data->request, buf);
				/* ereport(INFO,(errmsg("Buffer: %s\n\n", buf))); */
				rest_transfer(ai_service);
				*((char *) (ai_service->rest_response->data) + ai_service->rest_response->data_size) = '\0';

				/*
				 * ereport(INFO,(errmsg("Return: %s -%ld\n\n",
				 * (char*)(ai_service->rest_response->data), pk_col_value)));
				 */

				if (ai_service->rest_response->response_code == HTTP_OK)
				{
					datas = DirectFunctionCall2(json_object_field_text,
												CStringGetTextDatum(
																	(char *) (ai_service->rest_response->data)),
												PointerGetDatum(cstring_to_text("data")));
					first_choice = DirectFunctionCall2(json_array_element_text,
													   datas,
													   PointerGetDatum(0));
					return_text = DirectFunctionCall2(json_object_field_text,
													  first_choice,
													  PointerGetDatum(cstring_to_text("embedding")));

					/*
					 * ereport(INFO,(errmsg("Return1: %s \n\n",
					 * text_to_cstring(DatumGetTextPP(return_text)))));
					 */
					strcpy(ai_service->service_data->response,
						   text_to_cstring(DatumGetTextPP(datas)));

					strcpy(data, text_to_cstring(DatumGetTextPP(return_text)));
					removeNewlines(data);
					/* insert this data into the table */
					sprintf(query, "UPDATE %s SET %s = '%s' WHERE %s%s = %ld",
							get_option_value(ai_service->service_data->options, OPTION_STORE_NAME),
							EMBEDDINGS_COLUMN_NAME, data,
							get_option_value(ai_service->service_data->options, OPTION_STORE_NAME),
							PK_SUFFIX, pk_col_value);
					execute_query_spi(query, false /* read only */ );

					/* make way for the next call */
					ai_service->rest_response->data_size = 0;
					*((char *) (ai_service->rest_response->data) + ai_service->rest_response->data_size) = '\0';
				}
				else if (ai_service->rest_response->data_size == 0)
				{
					strcpy(ai_service->service_data->response, "Something is not ok, try again.");
					return;
				}

			}					/* end of while cursor */
		}						/* rows returned */
		SPI_finish();
		strcpy((char *) (ai_service->rest_response->data), get_option_value(ai_service->service_data->options, OPTION_STORE_NAME));
	}							/* create embeddings */

	if (ai_service->function_flags & FUNCTION_QUERY_VECTOR_STORE)
	{
		strcpy(ai_service->service_data->request, get_option_value(ai_service->service_data->options, OPTION_NL_QUERY));
		rest_transfer(ai_service);
		*((char *) (ai_service->rest_response->data) + ai_service->rest_response->data_size) = '\0';

		/*
		 * ereport(INFO,(errmsg("Return: %s\n\n",
		 * ai_service->rest_response->data)));
		 */

		if (ai_service->rest_response->response_code == HTTP_OK)
		{
			datas = DirectFunctionCall2(json_object_field_text,
										CStringGetTextDatum(
															(char *) (ai_service->rest_response->data)),
										PointerGetDatum(cstring_to_text("data")));
			first_choice = DirectFunctionCall2(json_array_element_text,
											   datas,
											   PointerGetDatum(0));
			return_text = DirectFunctionCall2(json_object_field_text,
											  first_choice,
											  PointerGetDatum(cstring_to_text("embedding")));

			strcpy(data, text_to_cstring(DatumGetTextPP(return_text)));
			removeNewlines(data);
			sprintf(query, "SELECT *, 1 - (%s <=> '%s') AS %s FROM %s ORDER BY cosine_similarity DESC ",
					EMBEDDINGS_COLUMN_NAME, data, EMBEDDINGS_COSINE_SIMILARITY,
					get_option_value(ai_service->service_data->options, OPTION_STORE_NAME));

			if (get_option_value(ai_service->service_data->options, OPTION_RECORD_COUNT))
			{
				strcat(query, "LIMIT ");
				strcat(query, get_option_value(ai_service->service_data->options, OPTION_RECORD_COUNT));
			}

			/* ereport(INFO,(errmsg("QUERY: %s \n\n", query))); */
			strcpy(ai_service->service_data->response, query);
			return;
		} //http response ok
	} //query embeddings
		return;
}
