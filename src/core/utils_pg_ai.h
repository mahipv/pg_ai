#ifndef _UTILS_PG_AI_H_
#define _UTILS_PG_AI_H_

#include <postgres.h>
#include <utils/builtins.h>
#include "executor/spi.h"

#include "ai_service.h"

/* input/ooput text manipulation helpers */
int escape_encode(const char *src, const size_t src_len, char *dst,
				  const size_t max_dst_len);
int get_word_count(const char *text, const size_t max_allowed,
				   size_t *actual_count);
void remove_new_lines(char *stream);

/* generic helper functions */
int is_extension_installed(const char *extension_name);
void make_pk_col_name(char *name, size_t max_len,
					  const char *vector_store_name);

/* JSON generation helpers */
void generate_json(char *buffer, const char *keys[], const char *values[],
				   const char *data_types[], size_t num_entries);

/* tuple manipulation helpers */
TupleDesc remove_columns(TupleDesc tupdesc, char **column_names,
						 int num_columns);
SPITupleTable *remove_columns_from_spitb(SPITupleTable *tuptable,
										 TupleDesc *result_tupdesc,
										 char *column_names[], int num_columns);

int execute_query_spi(const char *query, bool read_only);
void set_similarity_algorithm(ServiceOption *options);
void make_embeddings_query(char *query, const size_t max_query_length,
						   const char *vector_data, const char *store_name,
						   const char *embeddings_column_name,
						   const char *similarity_algorithm,
						   const char *similarity_alias);
int update_embeddings_vector_store(const int64 pk_col_value, const char *data,
								   const char *qualified_store_name);

#endif /* _UTILS_PG_AI_H_ */
