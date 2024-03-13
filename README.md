# pg_ai

AI extension for PostgreSQL to interpret and query data with RAG builtin.

## Description

The pg_ai extension harnesses AI models for two primary purposes:

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
- pg_ai uses libcurl for REST communication, make sure the curl library is installed.
- uses the [pgvector](https://github.com/pgvector/pgvector) extension for vector operations, make sure the extension is installed.


## Getting Started

```sql
CREATE EXTENSION pg_ai;
```

Set the [API Key](https://platform.openai.com/api-keys)
```sql
SET pg_ai.api_key='sk-********q';
```

1. Get the column data interpreted by the LLM.
```sql
SELECT col1, col2, pg_ai_insight(col1) AS insight FROM my_table WHERE id > 5;
```

2. Aggregate version of the above function.
```sql
SELECT pg_ai_insight_agg(col1, 'Suggest a topic name for the values') AS topic FROM my_table WHERE id > 5;
```

3. Create a vector store for a dataset
```sql
 SELECT pg_ai_create_vector_store(store => 'movies_vec_store_90s',
                           query => 'SELECT * FROM public.movies WHERE release_year > 1990',
                           notes => 'movies released after 1990');
```

4. Query the vector store
```sql
SELECT pg_ai_query_vector_store(store => 'movies_vec_store_90s',
                                prompt => 'movies on travel',
                                count=>3);
```

### Help
```sql
SELECT pg_ai_help();
```

## Notes

Currently supported service models.

1. OpenAI - gpt-3.5-turbo-instruct
2. OpenAI - dall-e2
3. OpenAI - ada(embeddings)

## TODOs

* Incorporate integration with additional local and remote LLMs.
* Implement parallelization and background loading of embedding vectors.
* Enhance support for curl read callbacks.
* Tokenization mechanism for larger data.
* Expand compatibility for other vector similarity algorithms & vector indexes.

## Disclaimer

This is not a official extension. All trademarks and copyrights are
the property of their respective owners.

The response received from the AI service is not interpreted or
modified by the extension, it is presented as received.