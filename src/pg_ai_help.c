#include <postgres.h>
#include <funcapi.h>
#include <utils/builtins.h>
#include "ai_service.h"

char	   *ai_services[] = {SERVICE_OPENAI, NULL};
char	   *open_ai_models[] = {
	MODEL_OPENAI_GPT,
	MODEL_OPENAI_EMBEDDINGS,
	MODEL_OPENAI_IMAGE_GEN,
	NULL
};


/*
 * The implementation of SQL FUNCTION pg_ai_help. Refer to the .sql file for
 * details on the parameters and return values.
 *
 */
PG_FUNCTION_INFO_V1(pg_ai_help);
Datum
pg_ai_help(PG_FUNCTION_ARGS)
{
	AIService  *ai_service;
	int			return_value;
	char	   *display_string;
	size_t		left_over;
	size_t		running_length;
	char	  **models = NULL;

	left_over = MAX_HELP_SIZE;


	running_length = 0;
	display_string = palloc0(MAX_HELP_SIZE);
	ai_service = palloc(sizeof(AIService));
	/* TODO handle strings and formatting and strlen efficiently */
	for (int i = 0; ai_services[i] != NULL; i++)
	{
		if (!strcmp(ai_services[i], SERVICE_OPENAI))
			models = open_ai_models;

		for (int j = 0; models[j] != NULL; j++)
		{
			/* clear all pointers */
			reset_service(ai_service);
			/* set all functions to the options defined */
			ai_service->function_flags |= ~0;
			/* initialize the service */
			return_value = initialize_service(ai_services[i], models[j], ai_service);
			if (return_value)
				PG_RETURN_TEXT_P(cstring_to_text("Internal error"));

			/* add service name */
			sprintf(display_string + running_length, "\n\n%c.\nService:", (j + 1 + '0'));
			running_length = strlen(display_string);
			sprintf(display_string + running_length, "%s  ", (ai_service->get_service_name) (ai_service));
			running_length = strlen(display_string);

			/* add service description */
			strcat(display_string + running_length, "Info:");
			running_length = strlen(display_string);
			sprintf(display_string + running_length, "%s\n",
					(ai_service->get_service_description) (ai_service));
			running_length = strlen(display_string);

			/* add model name */
			sprintf(display_string + running_length, "Model:");
			running_length = strlen(display_string);
			sprintf(display_string + running_length, "%s  ",
					(ai_service->get_model_name) (ai_service));
			running_length = strlen(display_string);
	
			/* add model description */
			strcat(display_string + running_length, "Info:");
			running_length = strlen(display_string);
			sprintf(display_string + running_length, "%s\n",
					(ai_service->get_model_description) (ai_service));
			running_length = strlen(display_string);

			/* get help text from respective service */
			(ai_service->get_service_help) (display_string + running_length, left_over - running_length);
			running_length = strlen(display_string);

			/* add the service specific parameters */
			strcat(display_string, "\nParameters:");
			running_length = strlen(display_string);
			get_service_options(ai_service, display_string + running_length,
								left_over - running_length);
			running_length = strlen(display_string);

			/* empty lines before the next service */
			strcat(display_string, "\n\n");
			running_length = strlen(display_string);
		}
	}
	PG_RETURN_TEXT_P(cstring_to_text(display_string));
}
