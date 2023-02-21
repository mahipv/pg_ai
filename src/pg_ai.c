#include <postgres.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/typcache.h>
#include "ai_service.h"
#include "rest_transfer.h"
#include "json_options_help.h"

/*
#if PG_VERSION_NUM < 140000
#error "Unsupported PostgreSQL version."
#endif
*/

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* supported ai services */
const char *ai_services[] =
{
	SERVICE_CHAT_GPT,
	SERVICE_DALL_E2,
	NULL
};

/*
 * The implementation of SQL FUNCTION get_insight. Refer to the .sql file for
 * details on the parameters and return values.
 */
PG_FUNCTION_INFO_V1(get_insight);
Datum
get_insight(PG_FUNCTION_ARGS)
{
	Name			service_name;
	char			*param_file_path;
	char		 	*request;
	AIService		*ai_service;
	int 			return_value;
	MemoryContext 	func_context;
	MemoryContext 	old_context;

	func_context = AllocSetContextCreate(CurrentMemoryContext,
										 "ai functions context",
										 ALLOCSET_DEFAULT_SIZES);
	old_context = MemoryContextSwitchTo(func_context);
	ai_service = palloc0(sizeof(AIService));
	/* clear all pointers */
	initialize_self(ai_service);

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
		ereport(FATAL,
				(errmsg("Incorrect parameters cannot proceed.\n")));

	/* get the call backs for the service */
	service_name = PG_GETARG_NAME(0);
	return_value = initialize_service(NameStr(*service_name),
									  ai_service);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

	/*
	  initialize the data for the service this is the place
	  to pass the user options to the service
	*/
	request = text_to_cstring(PG_GETARG_TEXT_PP(1));
	param_file_path = NameStr(*(PG_GETARG_NAME(2)));
	return_value = (ai_service->init_service_data)(request, ai_service, param_file_path);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot make options"));

	/* call the transfer */
	(ai_service->rest_transfer)(ai_service);

	MemoryContextSwitchTo(old_context);
	PG_RETURN_TEXT_P(cstring_to_text((char*)(ai_service->rest_response->data)));
}

/* structure to save the context for the aggregate function */
typedef struct AggStateStruct
{
	AIService 	*ai_service;
	char		column_data[16*1024];
	char		*param_file_path;
}AggStateStruct;

/*
 * The implementation of the aggregate transfer function, called once per
 * row. Refer the SQL FUNCTION _get_insight_agg_transfn in the .sql file for
 * details on the parameters and return value.
 *
 */
PG_FUNCTION_INFO_V1(get_insight_agg_transfn);
Datum
get_insight_agg_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext 		agg_context;
	MemoryContext 		old_context;
	AggStateStruct 		*state_struct;
	Name				service_name;

	if (PG_ARGISNULL(1) || PG_ARGISNULL(2) || PG_ARGISNULL(3))
		ereport(FATAL,
				(errmsg("Incorrect parameters cannot proceed.\n")));

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
		state_struct = (AggStateStruct *)palloc0(sizeof(AggStateStruct));
		*state_struct->column_data='\0';
		state_struct->ai_service = (AIService *)palloc0(sizeof(AIService));

		/* clear all pointers */
		initialize_self(state_struct->ai_service);

		/* initialize the service */
		service_name = PG_ARGISNULL(1) ? NULL : PG_GETARG_NAME(1);
		initialize_service(NameStr(*service_name),
						   state_struct->ai_service);
		state_struct->ai_service->is_aggregate = 0x1;
		state_struct->param_file_path = NameStr(*(PG_GETARG_NAME(3)));
		MemoryContextSwitchTo(old_context);
		fcinfo->flinfo->fn_extra = state_struct;
	}

	/* accumulate non NULL values */
	if (!PG_ARGISNULL(2))
	{
		strcat(state_struct->column_data, TextDatumGetCString(PG_GETARG_DATUM(2)));
		strcat(state_struct->column_data, " ");
		// ereport(INFO,(errmsg("%s\n",state_struct->column_data)));
	}

	PG_RETURN_POINTER(state_struct);
}

/*
 * The implementation of the aggregate final function, called only once after
 * all the rows are read. Refer the SQL FUNCTION _get_insight_agg_finalfn in
 * the .sql file for details on the parameters and return value.
 *
 */
PG_FUNCTION_INFO_V1(get_insight_agg_finalfn);
Datum
get_insight_agg_finalfn(PG_FUNCTION_ARGS)
{
	MemoryContext 		agg_context;
	MemoryContext 		old_context;
	AggStateStruct 		*state_struct;
	int					return_value;

	if (!AggCheckCallContext(fcinfo, &agg_context))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("called in non-aggregate context")));

	state_struct = (AggStateStruct *)PG_GETARG_POINTER(0);
	if (state_struct == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Internal Error"));

	if (state_struct->ai_service == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Internal Error"));

	/* the service was not initialized */
	if (state_struct->ai_service->get_service_name == NULL)
		PG_RETURN_TEXT_P(cstring_to_text("Unsupported service."));

	old_context = MemoryContextSwitchTo(agg_context);
	/*
	  initialize the data for the service this is the place
	  to pass the user options to the service
	*/
	return_value = (state_struct->ai_service->init_service_data)
		(state_struct->column_data, state_struct->ai_service,
		 state_struct->param_file_path);
	if (return_value)
		PG_RETURN_CSTRING("Internal error: cannot make options");

	// ereport(INFO,(errmsg("Final :%s\n",state_struct->column_data)));
	/* call the transfer */
	(state_struct->ai_service->rest_transfer)(state_struct->ai_service);

	MemoryContextSwitchTo(old_context);
	PG_RETURN_TEXT_P(cstring_to_text((char*)(state_struct->ai_service->rest_response->data)));
}

/*
 * The implementation of SQL FUNCTION pg_ai_help. Refer to the .sql file for
 * details on the parameters and return values.
 *
 */
PG_FUNCTION_INFO_V1(pg_ai_help);
Datum
pg_ai_help(PG_FUNCTION_ARGS)
{
	AIService	*ai_service;
	int			return_value;
	char		*display_string;
	size_t		left_over;
	size_t		running_length;

	left_over = MAX_HELP_SIZE;
	running_length = 0;
	display_string = palloc0(MAX_HELP_SIZE);

	ai_service = palloc(sizeof(AIService));
	/* TODO handle strings and formatting and strlen efficiently */
	for(int i=0; ai_services[i] != NULL; i++)
	{
		/* clear all pointers */
		initialize_self(ai_service);

		/* initialize the service */
		return_value = initialize_service(ai_services[i], ai_service);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Internal error"));

		/* add service name */
		sprintf(display_string + running_length, "%c.Service:", (i+1+'0'));
		running_length = strlen(display_string);
		sprintf(display_string + running_length, "%s ",
				(ai_service->get_service_name)(ai_service));
		running_length = strlen(display_string);

		/* add provider name */
		sprintf(display_string + running_length, "  Provider:");
		running_length = strlen(display_string);
		sprintf(display_string + running_length, "%s ",
				(ai_service->get_provider_name)(ai_service));
		running_length = strlen(display_string);

		/* add service description */
		strcat(display_string + running_length, "  Info: ");
		running_length = strlen(display_string);
		sprintf(display_string + running_length, "%s\n",
				(ai_service->get_service_description)(ai_service));
		running_length = strlen(display_string);

		/* get help text from respective service */
		(ai_service->get_service_help)(display_string+running_length,
											  left_over-running_length);
		running_length = strlen(display_string);

		/* add the service specific parameters */
		strcat(display_string, "\nJSON Parameters:");
		running_length = strlen(display_string);
		get_service_options(ai_service, display_string+running_length,
							left_over-running_length);
		running_length = strlen(display_string);

		/* empty lines before the next service */
		strcat(display_string, "\n\n\n");
		running_length = strlen(display_string);
	}
	strcat(display_string, "For json options file template - SELECT pg_ai_help_options();");
	PG_RETURN_TEXT_P(cstring_to_text(display_string));
}

/*
 * The implementation of SQL FUNCTION pg_ai_help_options. This decribes the
 * format of the json options file.Refer to the .sql file for details on the
 * parameters and return values.
 *
 */
PG_FUNCTION_INFO_V1(pg_ai_help_options);
Datum
pg_ai_help_options(PG_FUNCTION_ARGS)
{
	/* TODO construct the format dynamically */
	PG_RETURN_TEXT_P(cstring_to_text(JSON_OPTIONS_FILE_FORMAT));
}
