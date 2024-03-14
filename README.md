# pg_ai

PostgreSQL extension with builtin RAG capabilities, enabling the interpretation and querying of data through both natural language and SQL functions.

The pg_ai extension harnesses AI LLMs for:

(i) Facilitating natural language interaction with datasets:
Transforming query results from PostgreSQL tables into a vector store,
enabling interactions with the store through natural language.

(ii) Interpreting data stored in PostgreSQL tables using generative AI:
Leveraging LLMs' continual learning and evolving capabilities to gain fresh
insights into data, providing new interpretations over time and with each query
(Single Datum, Multiple Inferences SDMI),

## Build & Installation

```sh
cd pg_ai
make install
```
- pg_ai uses libcurl for communication with the AI services, make sure the curl is installed.
- uses the [pgvector](https://github.com/pgvector/pgvector) extension for vector operations, needs to be installed for vector operations.


## Getting Started

```sql
CREATE EXTENSION pg_ai;
```

Set the OpenAI [API Key](https://platform.openai.com/api-keys)
```sql
SET pg_ai.api_key='sk-********q';
```

Get the column data interpreted by LLM.
```sql
SELECT col1, col2, pg_ai_insight(col1) AS insight FROM my_table WHERE id > 5;
```

Aggregate version of the above function.
```sql
SELECT pg_ai_insight_agg(col1, 'Suggest a topic name for the values') AS topic
 FROM my_table WHERE id > 5;
```

Create vector store for a dataset.
```sql
 SELECT pg_ai_create_vector_store(store => 'movies_vec_store_90s',
                                  query => 'SELECT * FROM public.movies WHERE release_year > 1990',
                                  notes => 'movies released after 1990');
```

Query the vector store.
```sql
SELECT pg_ai_query_vector_store(store => 'movies_vec_store_90s',
                                prompt => 'movies on time travel',
                                count => 3);
```

### Help
```sql
SELECT pg_ai_help();
```

## More Supported Models

[Moderations](README_moderations.md)

[Text to Image](README_image_gen.md)

## Notes

Model versions in use.

1. OpenAI - gpt-3.5-turbo-instruct
2. OpenAI - text-embedding-ada-002
3. OpenAI - text-moderation-stable
4. OpenAI - dall-e-3

## TODO

* Implement parallelization and background loading of embedding vectors.
* Enhance support for curl read callbacks.
* Tokenization mechanism for larger data.
* Expand compatibility for other vector similarity algorithms & support vector indexes.
* Integration with additional local and remote LLMs.

## Disclaimer

This is not a official extension. All trademarks and copyrights are
the property of their respective owners.

The response received from the AI service is not interpreted or
modified by the extension, it is presented as received.