/* TODO construct the format dynamically */
#define JSON_OPTIONS_FILE_FORMAT \
"{\n\
   \"version\":\"1.0\",\n\
   \"providers\":[\n\
      {\n\
         \"name\":\"OpenAI\",\n\
         \"key\":\"<API Authorization KEY from your OpenAI account>\",\n\
         \"services\":[\n\
            {\n\
               \"name\":\"chatgpt\",\n\
               \"model\":\"text-davinci-003\",\n\
               \"prompt\":\"Get summary of the following in 1 lines.\",\n\
               \"promptagg\":\"Get a topic name for the words here.\"\n\
            },\n\
            {\n\
               \"name\":\"dalle-2\",\n\
               \"prompt\":\"Make a picture for the following \",\n\
               \"promptagg\":\"Make a picture with the following words\"\n\
            }\n\
         ]\n\
      },\n\
      {\n\
         \"name\":\"<provider2 version 2.0>\",\n\
         \"key\":\"\",\n\
         \"services\":[\n\
            {\n\
               \"name\":\"\",\n\
               \"model\":\"\",\n\
               \"prompt\":\"\",\n\
               \"promptagg\":\"\"\n\
            },\n\
            {\n\
               \"name\":\"\",\n\
               \"model\":\"\",\n\
               \"prompt\":\"\",\n\
               \"promptagg\":\"\"\n\
            }\n\
         ]\n\
      }\n\
   ]\n\
}"
