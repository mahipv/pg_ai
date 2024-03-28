#include <postgres.h>
#include <funcapi.h>
#include <utils/builtins.h>

#include "core/ai_service.h"

/*
 * Print the service and model information in the header.
 */
static size_t get_service_header(char *display_string, const size_t max_length,
								 const AIService *ai_service,
								 const size_t running_length)
{
	size_t print_length = 0;
	print_length =
		snprintf(display_string + running_length, max_length - running_length,
				 "\n\nService: %s  Info: %s\nModel: %s  Info: %s\n",
				 ai_service->get_service_name(ai_service),
				 ai_service->get_service_description(ai_service),
				 ai_service->get_model_name(ai_service),
				 ai_service->get_model_description(ai_service));
	return print_length;
}

/*
 * Get the help text from the service and add it to the display string.
 */
static size_t get_service_help(char *display_string, const size_t max_length,
							   const AIService *ai_service,
							   const size_t running_length)
{
	char *help_text;
	size_t print_length = 0;

	help_text =
		MemoryContextAlloc(ai_service->memory_context, MAX_HELP_TEXT_SIZE);

	ai_service->get_service_help(help_text, MAX_HELP_TEXT_SIZE);
	print_length =
		snprintf(display_string + running_length,
				 MAX_HELP_TEXT_SIZE - running_length, "%s", help_text);

	pfree(help_text);
	return print_length;
}

/*
 * Get the service options from each service and add them to the display string.
 */
static size_t get_service_options(char *display_string, const size_t max_length,
								  const AIService *ai_service,
								  const size_t running_length)
{
	char *service_options;
	size_t print_length = 0;

	service_options =
		MemoryContextAlloc(ai_service->memory_context, MAX_HELP_TEXT_SIZE);

	/* get the options in a string */
	print_service_options(ai_service->service_data->options,
						  false /* print_value */, service_options,
						  MAX_HELP_TEXT_SIZE);
	/* concat the options to the display string */
	print_length = snprintf(display_string + running_length,
							MAX_HELP_TEXT_SIZE - running_length,
							"\nParameters: %s\n\n", service_options);

	pfree(service_options);
	return print_length;
}

/*
 * Implementation of SQL FUNCTION pg_ai_help().
 */
PG_FUNCTION_INFO_V1(pg_ai_help);
Datum pg_ai_help(PG_FUNCTION_ARGS)
{
	AIService *ai_service = palloc(sizeof(AIService));
	char *display_string = palloc0(MAX_HELP_TEXT_SIZE);
	size_t running_length = 0;
	int service_flags = 0x1;
	int model_flags = 0x1;
	bool all_services_done = false;
	MemoryContext help_context;

	/* Loop through all the services and models and initialize them */
	while (!all_services_done)
	{
		model_flags = 0x1;
		while (model_flags)
		{
			/*
			 * Help will not need much memory, but the services are designed to
			 * allocate their own memory on initialization which when
			 * accumulated might become quite big. So we will create a new
			 * memory context for each service and delete it after the service
			 * is done.
			 */
			help_context = AllocSetContextCreate(
				CurrentMemoryContext, PG_AI_MCTX, ALLOCSET_DEFAULT_SIZES);
			ai_service->memory_context = help_context;

			/* set all function falgs to define and display all options */
			ai_service->function_flags |= ~0;
			if (initialize_service(service_flags, model_flags, ai_service))
			{
				/* if no models are supported then this service beyond last */
				if (model_flags & 0x01)
					all_services_done = true;
				/* go to next service */
				break;
			}

			/* Add the service and model information to the display string */
			running_length += get_service_header(
				display_string, MAX_HELP_TEXT_SIZE, ai_service, running_length);

			/* Add the help text returned by the service */
			running_length += get_service_help(
				display_string, MAX_HELP_TEXT_SIZE, ai_service, running_length);

			/* Add the service options to the display string */
			running_length += get_service_options(
				display_string, MAX_HELP_TEXT_SIZE, ai_service, running_length);

			model_flags = model_flags << 1; /* next model */

			if (ai_service->memory_context)
				MemoryContextDelete(ai_service->memory_context);
		}
		service_flags = service_flags << 1; /* next service */
	}

	pfree(ai_service);
	PG_RETURN_TEXT_P(cstring_to_text(display_string));
}
