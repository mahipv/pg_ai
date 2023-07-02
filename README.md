# pg_ai

PostgreSQL extension to interpret data with LLM based AI tools.

## Description

Introducing the #pg_ai extension, which utilizes LLM-based AI to
interpret data stored in PostgreSQL tables. Use the functions of this
extension in standard SQL to get insights about data.

Same data, multiple inferences (SDMI if I may say so :-)). As LLMs are
constantly learning, evolving, and are capable of interpreting data in
new ways, you can gain fresh insights into your data over time and
every time.

This extension #pg_ai implements the following functions for data insights

```sql
create_vector_store('<service>', '<options file>', '<new store_name>')

query_vector_store('<service>', '<options file>', '<store_name>', '<natural language prompt>')

get_insight('<service>', <column>, '<options file>')

get_insight_agg('<service>', <column>, '<options file>')
```

## Build & Installation

- pg_ai uses libcurl for REST communication, make sure the curl library is installed.
- uses the pgvector extension for vector operations, make sure the extension is installed.
- check that pg_config is installed and available in the system's PATH.
- Use of embeddings/vector functions depends on the 'pg_vector' extension.
```sh
cd pg_ai
make install
```

## Usage

There are limitations on the amount of text(tokens) that can be passed
to the LLM models. Use these functions with columns of smaller width.

Examples:

```sql
CREATE EXTENSION pg_ai;
```

### Function ```create_vector_store()```
```sql
postgres=# SELECT create_vector_store('ada', '/home/pg_ai.json', 'movie_lake_1');
INFO:
Processing query ... "SELECT * FROM public.movies WHERE release_year > 1990"

 create_vector_store
---------------------
 movie_lake_1
(1 row)
```

### Function ```query_vector_store()```

Query the store with prompt(string) not necssarily matching the data. This is supposed to bring out the related records.

```sql
postgres=# SELECT query_vector_store('ada',  '/home/pg_ai.json', 'movie_lake_1', 'hulk');

INFO:  Meta Data ( id  name  release_year  genre  summary )
query_vector_store
-------------------
 (20,"Avengers: Endgame",2019,Action,"After the devastating events of Avengers: Infinity War, the universe is in ruins. With the help of remaining allies, the Avengers assemble once more to undo Thanos' actions and restore order to the universe.")
(1 row)



postgres=# SELECT query_vector_store('ada',  '/home/pg_ai.json', 'movie_lake_1', 'iceberg');

INFO:  Meta Data ( id  name  release_year  genre  summary )
query_vector_store
-------------------
 (8,Titanic,1997,Romance,"A seventeen-year-old aristocrat falls in love with a kind but poor artist aboard the luxurious, ill-fated R.M.S. Titanic.")
(1 row)



postgres=# SELECT query_vector_store('ada',  '/home/pg_ai.json', 'movie_lake_1', 'maximus');

INFO:  Meta Data ( id  name  release_year  genre  summary )
 query_vector_store
-------------------
 (12,Gladiator,2000,Action,"A former Roman General sets out to exact vengeance against the corrupt emperor who murdered his family and sent him into slavery.")
(1 row)
```

### Function ```get_insight()```

```sql
postgres=# SELECT entry FROM information WHERE id > 0 AND id < 5;
       entry
-------------------
 System R
 Dihydrogen Oxide
 TimescaleDB
 webbtelescope.org
(4 rows)
```

To get insights for the above data from ChatGPT

```sql
postgres=# SELECT entry, get_insight('chatgpt', entry, '/home/pg_ai.json') AS Insight FROM information WHERE id > 0 AND id < 5;
       entry       |                                                                     insight
-------------------+--------------------------------------------------------------------------------------------------------------------------------------------------
 System R          |   System R is an IBM database system research project, which initiated the development of relational database technologies.
 Dihydrogen Oxide  |   Dihydrogen Oxide is another name for Water.
 TimescaleDB       |   TimescaleDB is an open-source relational database built for analyzing time-series data with the power and convenience of SQL.
 webbtelescope.org |   webbtelescope.org is a website providing information about the James Webb Space Telescope, an international space telescope launching in 2021.
(4 rows)

```

### Function ```get_insight_agg()```

Say a table 'chat' has the following data in the 'message' column

```sql
postgres=# SELECT message FROM chat WHERE ts::date = date '2023-01-01';
          message
---------------------------
 brake, tyres
 Fuel, spark plug
 four wheel drive
  engine, gear box
  axle air filter
  air conditioning
  detailing, gear ratio
 Carburettor,  seat covers
(8 rows)
```

To get an aggregated insight of the above data, use the aggregate version of the function with ChatGPT

```sql
postgres=# SELECT get_insight_agg('chatgpt', message, '/home/pg_ai.json') AS "Topic" FROM chat WHERE ts::date = date '2023-01-01';
                        Topic
------------------------------------------------------
   "Maintenance and Performance of Four-Wheel Drives"
(1 row)
```

To generate a picture based on the above data using Dall-E2

```sql
postgres=# SELECT get_insight_agg('dall-e2', message, '/home/pg_ai.json') AS "pic" FROM chat WHERE ts::date = date '2023-01-01';
                                pic
--------------------------------------------------------------------
 https://***leapiprodscus.blob.core.****.net/private/org-fTI20YbDph2gXaSUgf3EV*** ...
(1 row)
```

### Help
```sql
SELECT pg_ai_help();
```

### The options file
```sql
SELECT pg_ai_help_options();
```

Make sure the API key from your OpenAI account is set correctly in the below json options.

```json
{
   "version":"1.0",
   "providers":[
      {
         "name":"OpenAI",
         "key":"<API authorization KEY of your OpenAI account>",
         "services":[
            {
               "name":"chatgpt",
               "prompt":"Get summary of the following in 1 lines.",
               "promptagg":"Get a topic name for the words here."
            },
            {
               "name":"dall-e2",
               "prompt":"Make a picture for the following ",
               "promptagg":"Make a picture with the following words"
            },
            {
               "name":"ada",
               "query": "<SELECT query to create the vector store>",
               "prompt": "The input string along with the query data",
               "algorithm": "<vector matching algorithm(cosine_similarity)>",
               "limit": "<No of matching data records to be fetched>"
            }
         ]
      },
      {
         "name":"<Provider2>",
         "key":"",
         "services":[
            {
               "name":"",
               "model":"",
               "prompt":"",
               "promptagg":""
            },
            {
               "name":"",
               "model":"",
               "prompt":"",
               "promptagg":""
            }
         ]
      }
   ]
}
```

```sql
DROP EXTENSION pg_ai;
```

## Notes

Currently supported models.

1. OpenAI - ChatGPT
2. OpenAI - Dall-E2
3. OpenAI - ADA(embeddings)

## TODOs

* Incorporate integration with additional local and remote LLMs.
* Implement parallelization and background loading of embedding vectors.
* Enhance support for curl read callbacks.
* Develop tokenization mechanism for larger row data.
* Expand compatibility for other vector similarity algorithms.

## Disclaimer

This is not a official extension. All trademarks and copyrights are
the property of their respective owners.

The response received from the AI service is not interpreted or
modified by the extension, it is presented as received.