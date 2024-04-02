#include "utils_pg_ai.h"

#include <funcapi.h>

#include "guc/pg_ai_guc.h"

/*
 * Function to escape the special characters that are to be sent as data
 * to a REST service. Make sure the dst array passed to this method can
 * accomodate the source string along with the escaped characters.
 */
int escape_encode(const char *src, const size_t src_len, char *dst,
				  const size_t max_dst_len)
{
	const char *p = src;
	char *q = dst;
	size_t src_index = 0;
	size_t dst_index = 0;
	int byte_count;

	while (src_index++ < src_len)
	{
		if (isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~')
		{
			*q++ = *p;
			dst_index++;
		}
		else
		{
			byte_count = sprintf(q, "%%%02X", (unsigned char)*p);

			/* if dst buffer cannot handle the new length return error */
			if (dst_index + byte_count > max_dst_len)
				return 1;

			q += byte_count;
			dst_index += byte_count;
		}
		p++;
	}
	*q = '\0';
	return 0;
}

/*
 * Given a string, get the number of words. If the word count is more
 * than a given max threshold, the counting is stopped.
 */
int get_word_count(const char *text, const size_t max_allowed,
				   size_t *actual_count)
{
	size_t count = 0;
	int i = 0;

	while (text[i] != '\0' && count < max_allowed)
	{
		if (text[i] == ' ' && text[i + 1] != ' ')
			count++;
		i++;
	}

	if (count < max_allowed)
	{
		if (actual_count)
			*actual_count = count;
		return RETURN_ZERO;
	}

	return RETURN_ERROR;
}

/*
 * Function to remove the given columns from the given TupleDesc.
 */
TupleDesc remove_columns(TupleDesc tupdesc, char **column_names,
						 int num_columns)
{
	int num_attrs = tupdesc->natts;
	int new_num_attrs = num_attrs - num_columns;
	AttrNumber *new_attnos = palloc0(new_num_attrs * sizeof(AttrNumber));
	int new_attr_index = 0;
	TupleDesc new_tupdesc;

	for (int i = 0; i < num_attrs; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
		bool remove_column = false;

		for (int j = 0; j < num_columns; j++)
		{
			if (strcmp(NameStr(attr->attname), column_names[j]) == 0)
			{
				remove_column = true;
				break;
			}
		}

		if (!remove_column)
		{
			new_attnos[new_attr_index] = attr->attnum;
			new_attr_index++;
		}
	}

	new_tupdesc = CreateTemplateTupleDesc(new_num_attrs);
	for (int i = 0; i < new_num_attrs; i++)
	{
		int old_attr_index = new_attnos[i] - 1;
		Form_pg_attribute old_attr = TupleDescAttr(tupdesc, old_attr_index);

		TupleDescInitEntry(new_tupdesc, i + 1, NameStr(old_attr->attname),
						   old_attr->atttypid, old_attr->atttypmod,
						   old_attr->attndims);
	}
	return new_tupdesc;
}

/*
 * Function to remove the given columns from the given SPITupleTable.
 */
SPITupleTable *remove_columns_from_spitb(SPITupleTable *tuptable,
										 TupleDesc *result_tupdesc,
										 char *column_names[], int num_columns)
{
	TupleDesc tupdesc = *result_tupdesc;
	int num_attrs = tupdesc->natts;
	int num_rows = tuptable->numvals;
	int new_num_attrs = num_attrs - num_columns;
	TupleDesc new_tupdesc;

	/* Datum **new_values = palloc0(new_num_attrs * sizeof(Datum *)); */
	/* bool **new_nulls = palloc0(new_num_attrs * sizeof(bool *)); */
	SPITupleTable *new_tuptable =
		(SPITupleTable *)palloc(sizeof(SPITupleTable));

	new_tuptable->tuptabcxt = tuptable->tuptabcxt;
	new_tuptable->alloced = true;
	new_tuptable->numvals = num_rows;
	new_tuptable->vals = palloc0(num_rows * sizeof(HeapTuple));
	new_tupdesc = remove_columns(tupdesc, column_names, num_columns);

	for (int i = 0; i < num_rows; i++)
	{
		HeapTuple old_tuple = tuptable->vals[i];
		Datum *new_row_values = palloc0(new_num_attrs * sizeof(Datum));
		bool *new_row_nulls = palloc0(new_num_attrs * sizeof(bool));

		int new_attr_index = 0;

		for (int j = 0; j < num_attrs; j++)
		{
			Form_pg_attribute attr = TupleDescAttr(tupdesc, j);
			bool remove_column = false;

			for (int k = 0; k < num_columns; k++)
			{
				if (strcmp(NameStr(attr->attname), column_names[k]) == 0)
				{
					remove_column = true;
					break;
				}
			}
			if (!remove_column)
			{
				new_row_values[new_attr_index] = SPI_getbinval(
					old_tuple, tupdesc, j + 1, &new_row_nulls[new_attr_index]);
				new_attr_index++;
			}
		}
		new_tuptable->vals[i] =
			heap_form_tuple(new_tupdesc, new_row_values, new_row_nulls);
	}
	*result_tupdesc = BlessTupleDesc(new_tupdesc);
	return new_tuptable;
}

/*
 * Function to check if a given extension is installed in the database.
 */
int is_extension_installed(const char *ext_name)
{
	char query[SERVICE_DATA_SIZE];
	int ret;
	int extension_exists = 0;

	snprintf(query, SERVICE_DATA_SIZE,
			 "SELECT 1 FROM pg_extension "
			 "WHERE extname = '%s'",
			 ext_name);
	SPI_connect();
	ret = SPI_exec(query, 0);
	if (ret > 0 && SPI_tuptable != NULL && SPI_processed > 0)
	{
		/* Extension exists */
		extension_exists = 1;
	}
	SPI_finish();
	return extension_exists;
}

/*
 * Function to generate a primary key column name from the given vector store
 * name.
 */
void make_pk_col_name(char *name, size_t max_len, const char *vector_store_name)
{
	snprintf(name, max_len, "%s%s", vector_store_name, PK_SUFFIX);
}

/*
 * Function to generate a JSON string from the given keys, values and data
 * types.
 */
void generate_json(char *buffer, const char *keys[], const char *values[],
				   const char *data_types[], size_t num_entries)
{
	/* Ensure the buffer is empty */
	buffer[0] = '\0';
	strcat(buffer, "{");

	for (size_t i = 0; i < num_entries; i++)
	{
		/* Add key */
		strcat(buffer, "\"");
		strcat(buffer, keys[i]);
		strcat(buffer, "\":");

		/* Check data type and handle accordingly */
		if (strcmp(data_types[i], "string") == 0)
		{
			strcat(buffer, "\"");
			strcat(buffer, values[i]);
			strcat(buffer, "\"");
		}
		else
			strcat(buffer, values[i]);

		/* Add comma if not the last entry */
		if (i < num_entries - 1)
			strcat(buffer, ",");
	}
	strcat(buffer, "}");
}

/*
 * Function to remove new lines and spaces from a given stream.
 */
void remove_new_lines(char *stream)
{
	char *p = stream;
	char *q = stream;

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

	/* Add the null terminator to mark the end of the modified stream */
	*q = '\0';
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
 * Function that prepares the select query for the embeddings service.
 * The query is based on the similarity algorithm set by the user.
 */
void make_embeddings_query(char *query, const size_t max_query_length,
						   const char *vector_data, const char *store_name,
						   const char *similarity_algo)
{
	/* refer pgvector docs for cosine syntax */
	if (!strcasecmp(similarity_algo, EMBEDDINGS_SIMILARITY_COSINE))
	{
		snprintf(query, max_query_length,
				 "SELECT *, 1 - (%s <=> '%s') AS %s FROM %s ORDER BY %s DESC ",
				 EMBEDDINGS_COLUMN_NAME, vector_data,
				 OPTION_SIMILARITY_ALGORITHM, store_name,
				 OPTION_SIMILARITY_ALGORITHM);
	}

	/* refer pgvector docs for euclidean distance syntax */
	if (!strcasecmp(similarity_algo, EMBEDDINGS_SIMILARITY_EUCLIDEAN))
	{
		snprintf(query, max_query_length,
				 "SELECT *, (%s <-> '%s') AS %s FROM %s ORDER BY %s ASC ",
				 EMBEDDINGS_COLUMN_NAME, vector_data,
				 OPTION_SIMILARITY_ALGORITHM, store_name,
				 OPTION_SIMILARITY_ALGORITHM);
	}

	/* refer pgvector docs for the inner product syntax */
	if (!strcasecmp(similarity_algo, EMBEDDINGS_SIMILARITY_INNER_PRODUCT))
	{
		snprintf(
			query, max_query_length,
			"SELECT *, (-1 * (%s <#> '%s')) AS %s FROM %s ORDER BY %s DESC ",
			EMBEDDINGS_COLUMN_NAME, vector_data, OPTION_SIMILARITY_ALGORITHM,
			store_name, OPTION_SIMILARITY_ALGORITHM);
	}
}