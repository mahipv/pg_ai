#include <postgres.h>
#include <funcapi.h>
#include <utils/builtins.h>

#include "ai_service.h"

/*
 * Implementation of SQL FUNCTION pg_ai_help().
 */
PG_FUNCTION_INFO_V1(pg_ai_help);
Datum pg_ai_help(PG_FUNCTION_ARGS)
{
	AIService *ai_service = palloc(sizeof(AIService));
	char *display_string = palloc0(MAX_HELP_TEXT_SIZE);
	size_t running_length = 0;
	int return_value = 0;
	char *help_text;
	char *service_options;
	int service_flags = 0x1;
	int model_flags = 0x1;
	bool all_services_done = false;
	int count = 1;
	MemoryContext help_context;

	/* Create a new memory context for the help text */
	help_context = AllocSetContextCreate(
		CurrentMemoryContext, "AI Help Context", ALLOCSET_DEFAULT_SIZES);
	ai_service->memory_context = help_context;

	/* Loop through all the services and models and initialize them */
	while (!all_services_done)
	{
		model_flags = 0x1;
		while (model_flags)
		{
			/* set all function falgs to define and display all options */
			ai_service->function_flags |= ~0;
			return_value =
				initialize_service(service_flags, model_flags, ai_service);
			if (return_value) /* no service */
			{
				/* if no models are supported then this service beyond last */
				if (model_flags & 0x01)
					all_services_done = true;
				/* go to next service */
				break;
			}

			/* Add the service and model information to the display string */
			running_length += snprintf(
				display_string + running_length,
				MAX_HELP_TEXT_SIZE - running_length,
				"\n\n%c.\nService: %s  Info: %s\nModel: %s  Info: %s\n",
				(count++ + '0'), ai_service->get_service_name(ai_service),
				ai_service->get_service_description(ai_service),
				ai_service->get_model_name(ai_service),
				ai_service->get_model_description(ai_service));

			/* Add the help text returned by the service */
			help_text = MemoryContextAlloc(ai_service->memory_context,
										   MAX_HELP_TEXT_SIZE);
			ai_service->get_service_help(help_text, MAX_HELP_TEXT_SIZE);
			running_length +=
				snprintf(display_string + running_length,
						 MAX_HELP_TEXT_SIZE - running_length, "%s", help_text);
			pfree(help_text);

			/* Add the service options to the display string */
			service_options = MemoryContextAlloc(ai_service->memory_context,
												 MAX_HELP_TEXT_SIZE);
			print_service_options(ai_service->service_data->options,
								  false /* print_value */, service_options,
								  MAX_HELP_TEXT_SIZE);
			running_length += snprintf(display_string + running_length,
									   MAX_HELP_TEXT_SIZE - running_length,
									   "\nParameters: %s\n\n", service_options);
			pfree(service_options);

			model_flags = model_flags << 1; /* next model */
		}
		service_flags = service_flags << 1; /* next service */
	}

	if (ai_service->memory_context)
		MemoryContextDelete(ai_service->memory_context);
	pfree(ai_service);
	PG_RETURN_TEXT_P(cstring_to_text(display_string));
}
