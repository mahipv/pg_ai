<p align="center">
  <img src="res/main-logo-transparent.svg" alt="PgAi" height="360" width="360" />
</p>

# pg_ai

PostgreSQL extension with builtin RAG capabilities, enabling the interpretation and querying of data through both natural language and SQL functions.

## Overview

The pg_ai extension brings the power of AI and LLMs to SQL. pg_ai helps in:

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
- pg_ai uses libcurl for communication with remote AI services, curl needs to be installed.
- needs [pgvector](https://github.com/pgvector/pgvector) extension for vector operations.


## Getting Started (the pg_ai_* functions)

```sql
CREATE EXTENSION pg_ai;
```

Choose the AI service 
```sql
SET pg_ai.service = 'OPenAI'(default)|'Gemini';
```

Set the API Key [OpenAI](https://platform.openai.com/api-keys) or [Google AI Studio](https://aistudio.google.com/app/apikey)
```sql
SET pg_ai.api_key='qz********y';
```

#### Insights

Get the column data interpreted by LLM.
```sql
SELECT col1, col2, pg_ai_insight(col1) AS insight FROM my_table WHERE id > 5;
```

Aggregate version of the above function.
```sql
SELECT pg_ai_insight_agg(col1, 'Suggest a topic for these values') AS topic FROM my_table WHERE id > 5;
```

#### Vectors

Create vector store for a dataset.
```sql
 SELECT pg_ai_create_vector_store(store => 'movies_vec_store_90s',
                                  query => 'SELECT * FROM public.movies WHERE release_year > 1990',
                                  notes => 'movies released after 1990');
```

Query the vector store with a natural language prompt.
```sql
SELECT pg_ai_query_vector_store(store => 'movies_vec_store_90s',
                                prompt => 'movies on time travel',
                                count => 3);
```

```sql
SET pg_ai.similarity_algorithm='cosine'(default)|'euclidean'|'inner_product';
```

#### Moderations

Get the moderations for the column data.
```sql
SELECT col1, pg_ai_moderation(col1, NULL) FROM messages_table WHERE id=1;
```

Aggregate version of the above function.
```sql
SELECT pg_ai_moderation_agg(col1, NULL) FROM messages_table WHERE id<10;
```

#### More functions and supported models
[Text to Image](README_image_gen.md)


### Help
```sql
SELECT pg_ai_help();
```

## Notes

Models in use.

1. OpenAI - gpt-3.5-turbo-instruct
2. OpenAI - text-embedding-ada-002
3. OpenAI - text-moderation-stable
4. OpenAI - dall-e-3
5. Google AI- gemini-pro:generateContent

## TODO

* Develop parallelization framework for loading embedding vectors in the background.
* Improve curl read callbacks and add tokenization support to handle longer context data.
* Enable customization to utilize alternative local and remote LLMs.
* Separate storage layer to enable the creation and querying of remote vector stores,(object stores/dbs, file-based storage).
* Explore new vector index type (HNTW).
* Introduce the extension catalog.

## Disclaimer

This is not a official extension. All trademarks and copyrights are
the property of their respective owners.

The response received from the AI service is not interpreted or
modified by the extension, it is presented as received.