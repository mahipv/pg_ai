#include "rest_transfer.h"

#include <postgres.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/typcache.h>
#include "ai_service.h"

#if PG_VERSION_NUM < 150000
#error "Unsupported PostgreSQL version."
#endif

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* supported ai services */
const char *ai_services[] =
{
	SERVICE_CHAT_GPT,
	SERVICE_DALLE_2,
	NULL
};

PG_FUNCTION_INFO_V1(get_insights);
Datum
get_insights(PG_FUNCTION_ARGS)
{
	Name			service_name;
	char			*auth_key;
	char		 	*request;
	AIService		*ai_service;
	int 			return_value;
	MemoryContext 	func_context;
	MemoryContext 	old_context;

	func_context = AllocSetContextCreate(CurrentMemoryContext,
										 "ai functions context",
										 ALLOCSET_DEFAULT_SIZES);
	old_context = MemoryContextSwitchTo(func_context);
	ai_service = palloc(sizeof(AIService));
	/* clear all pointers */
	initialize_self(ai_service);

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
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
	  to pass the user params to the service
	*/
	request = text_to_cstring(PG_GETARG_TEXT_PP(1));
	auth_key = PG_ARGISNULL(2) ? NULL : NameStr(*(PG_GETARG_NAME(2)));
	return_value = (ai_service->init_service_data)(request,
												   ai_service, auth_key);
	if (return_value)
		PG_RETURN_TEXT_P(cstring_to_text("Internal error: cannot make params"));

	/* call the transfer */
	(ai_service->rest_transfer)(ai_service);

	MemoryContextSwitchTo(old_context);
	PG_RETURN_TEXT_P(cstring_to_text((char*)(ai_service->rest_response->data)));
}

PG_FUNCTION_INFO_V1(get_insights_agg_transfn);
typedef struct AggStateStruct
{
	AIService 	*ai_service;
	char		column_data[16*1024];
	char		*auth_key;
}AggStateStruct;

Datum
get_insights_agg_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext 		agg_context;
	MemoryContext 		old_context;
	AggStateStruct 		*state_struct;
	Name				service_name;
	int					return_value;

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
		return_value = initialize_service(NameStr(*service_name),
										  state_struct->ai_service);
		state_struct->ai_service->is_aggregate = 0x1;
		if (return_value)
			PG_RETURN_CSTRING("Unsupported service.");

		state_struct->auth_key = PG_ARGISNULL(3) ? NULL : NameStr(*(PG_GETARG_NAME(3)));
		/* TODO pass default authkey to agg function */
		if (strlen(state_struct->auth_key) == 0)
			state_struct->auth_key = NULL;
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

PG_FUNCTION_INFO_V1(get_insights_agg_finalfn);
Datum
get_insights_agg_finalfn(PG_FUNCTION_ARGS)
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
	old_context = MemoryContextSwitchTo(agg_context);
	/*
	  initialize the data for the service this is the place
	  to pass the user params to the service
	*/
	return_value = (state_struct->ai_service->init_service_data)
		(state_struct->column_data, state_struct->ai_service,
			state_struct->auth_key);
	if (return_value)
		PG_RETURN_CSTRING("Internal error: cannot make params");

	// ereport(INFO,(errmsg("Final :%s\n",state_struct->column_data)));
	/* call the transfer */
	(state_struct->ai_service->rest_transfer)(state_struct->ai_service);
	MemoryContextSwitchTo(old_context);
	PG_RETURN_TEXT_P(cstring_to_text((char*)(state_struct->ai_service->rest_response->data)));
}

#define MAX_HELP_SIZE 1*1024
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
	for(int i=0; ai_services[i] != NULL; i++)
	{
		/* clear all pointers */
		initialize_self(ai_service);
		return_value = initialize_service(ai_services[i], ai_service);
		if (return_value)
			PG_RETURN_TEXT_P(cstring_to_text("Internal error"));

		/* TODO handle strings and formatting in a better way */
		sprintf(display_string + running_length, "%c.Service: ", (i+1+'0'));
		running_length = strlen(display_string);

		(ai_service->get_service_name)(display_string+running_length,
									   left_over-running_length);
		running_length = strlen(display_string);

		strcat(display_string + running_length, "   Info: ");
		running_length = strlen(display_string);

		(ai_service->get_service_description)(display_string+running_length,
											  left_over-running_length);
		running_length = strlen(display_string);

		(ai_service->get_service_help)(display_string+running_length,
											  left_over-running_length);
		running_length = strlen(display_string);

	}
	PG_RETURN_TEXT_P(cstring_to_text(display_string));
}
