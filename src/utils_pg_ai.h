#ifndef _UTILS_PG_AI_H_
#define _UTILS_PG_AI_H_

#include <ctype.h>
#include <stdio.h>
#include <postgres.h>
#include <utils/builtins.h>
#include "executor/spi.h"

int			escape_encode(const char *src, const size_t src_len,
						  char *dst, const size_t max_dst_len);
int			get_word_count(const char *text, const size_t max_allowed, size_t *actual_count);
TupleDesc	remove_columns(TupleDesc tupdesc, char **column_names, int num_columns);
SPITupleTable *remove_columns_from_spitb(SPITupleTable *tuptable, TupleDesc *result_tupdesc,
										 char *column_names[], int num_columns);
int			is_extension_installed(const char *extension_name);
void		make_pk_col_name(char *name, size_t max_len, const char *vector_store_name);
void		generate_json(char *buffer, const char *keys[], const char *values[],
						  const char *data_types[], size_t num_entries);
#endif							/* _UTILS_PG_AI_H_ */
