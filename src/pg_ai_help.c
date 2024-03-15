#include <postgres.h>
#include <funcapi.h>
#include <utils/builtins.h>
#include "ai_service.h"

/* supported services and models */
char	   *ai_services[] = {SERVICE_OPENAI, NULL};
char	   *open_ai_models[] = {
	MODEL_OPENAI_GPT,
	MODEL_OPENAI_EMBEDDINGS,
	MODEL_OPENAI_MODERATION,
	MODEL_OPENAI_IMAGE_GEN,
	NULL
};


/*
 * Implementation of SQL FUNCTION pg_ai_help().
 */
PG_FUNCTION_INFO_V1(pg_ai_help);
Datum
pg_ai_help(PG_FUNCTION_ARGS)
{
	AIService  *ai_service = palloc(sizeof(AIService));
	char	   *display_string = palloc0(MAX_HELP_TEXT_SIZE);
	size_t		running_length = 0;
	int			return_value = 0;
	char	   *help_text;
	char	   *service_options;
	MemoryContext 	help_context;
	
	/* Create a new memory context for the help text */
	help_context = AllocSetContextCreate(CurrentMemoryContext,
										"AI Help Context",
										ALLOCSET_DEFAULT_SIZES);
	ai_service->memory_context = help_context;
	
	for (int i = 0; ai_services[i] != NULL; i++)
	{
		char	  **models = NULL;

		if (!strcmp(ai_services[i], SERVICE_OPENAI))
			models = open_ai_models;

		for (int j = 0; models && models[j] != NULL; j++)
		{
			/* set all function falgs to define all options and can be displayed */
			ai_service->function_flags |= ~0;

			return_value = initialize_service(ai_services[i], models[j], ai_service);
			if (return_value)
			{
				pfree(ai_service);
				pfree(display_string);
				if (ai_service->memory_context)
					MemoryContextDelete(ai_service->memory_context);
				PG_RETURN_TEXT_P(cstring_to_text("Internal error"));
			}

			/* Add the service and model information to the display string */
			running_length += snprintf(display_string + running_length,
									   MAX_HELP_TEXT_SIZE - running_length,
									   "\n\n%c.\nService: %s  Info: %s\nModel: %s  Info: %s\n",
									   (j + 1 + '0'), ai_service->get_service_name(ai_service),
									   ai_service->get_service_description(ai_service),
									   ai_service->get_model_name(ai_service),
									   ai_service->get_model_description(ai_service));

			help_text = MemoryContextAlloc(ai_service->memory_context, MAX_HELP_TEXT_SIZE);
			ai_service->get_service_help(help_text, MAX_HELP_TEXT_SIZE);
			running_length += snprintf(display_string + running_length,
									   MAX_HELP_TEXT_SIZE - running_length,
									   "%s", help_text);
			pfree(help_text);

			service_options = MemoryContextAlloc(ai_service->memory_context, MAX_HELP_TEXT_SIZE);
			get_service_options(ai_service, service_options, MAX_HELP_TEXT_SIZE);
			running_length += snprintf(display_string + running_length,
									   MAX_HELP_TEXT_SIZE - running_length,
									   "\nParameters: %s\n\n", service_options);
			pfree(service_options);
		}
	}
	if (ai_service->memory_context)
		MemoryContextDelete(ai_service->memory_context);
	pfree(ai_service);
	PG_RETURN_TEXT_P(cstring_to_text(display_string));
}
