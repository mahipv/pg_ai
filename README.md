# pg_ai

PostgreSQL extension to fetch data using natural language and interpret data with LLM based AI tools.

## Description

Introducing the pg_ai extension, which utilizes LLM-based AI models to

(i) Use natural language to interact with datasets.
Transform result set from the queries on PostgreSQL tables into a
vector store. Engage in interactions with the store using natural
language.

(ii) interpret data stored in PostgreSQL tables using generative AI.
Single Datum, Multiple Inferences (SDMI :-)). As LLMs are
constantly learning, evolving, and are capable of interpreting data in
new ways, gain fresh insights into your data over time and every time.

## pg_ai Functions

```sql
1. create_vector_store(service => '<AI Service name>',
                       key => '<auth key for the AI Service>',
                       store => '<name of the vector store to be created>',
                       query => '<SQL query, result of which is stored in the vector store>',
                       prompt => '<any additional string describing the result set>')
```
```sql
2. query_vector_store(service => '<AI Service name>',
                      key => '<auth key for the AI Service>',
                      store => '<name of the vector store to be queried>',
                      prompt => '<natural language query>',
                      count => <no of records to be fetched>[default 1])
```
```sql
3. get_insight(service => '<AI Service name>',
               key => '<auth key for the AI Service>',
               textcolumn => <column name - value of which will be interpreted by the AI service>,
               prompt => '<Language prompt to the AI service>')
```
```sql
4. get_insight_agg('<AI Service name>',
                   '<auth key for the AI Service>',
                   <column name - concatenated values of which will be interpreted by the AI service>,
                   <Language prompt to the AI service>)
```

## Build & Installation

- pg_ai uses libcurl for REST communication, make sure the curl library is installed.
- uses the [pgvector](https://github.com/pgvector/pgvector) extension for vector operations, make sure the extension is installed.
- check that pg_config is installed and available in the system's PATH.
- Use of embeddings/vector functions depends on the 'pg_vector' extension.

```sh
cd pg_ai
make install
```

## Examples

When utilizing LLM models, it is important to consider the limitations
on text/token length and volume. This extension does not support
tokenization. It is advisable to use these functions with smaller data strings.

```sql
CREATE EXTENSION pg_ai;
```

### 1. Function ```create_vector_store()```

```sql
postgres=# SELECT create_vector_store(service => 'ada',
                                      key => '<OpenAI auth key>',
                                      store => 'movie_lake_1',
                                      query => 'SELECT movies.name, movies.release_year, movies.genre, movies.summary,
                                                movie_details.director, movie_details.production_house
                                                FROM movies INNER JOIN movie_details ON movies.name = movie_details.name
                                                AND movies.release_year > 1990',
                                      prompt => 'movie details');

INFO: Processing query ...

 create_vector_store
---------------------
 movie_lake_1
(1 row)
```

Tables with following definition are used by the query above.

```sql
postgres=# \d movies

                         Table "public.movies"
    Column    |          Type          | Collation | Nullable | Default
--------------+------------------------+-----------+----------+---------
 id           | integer                |           | not null |
 name         | character varying(255) |           |          |
 release_year | integer                |           |          |
 genre        | character varying(255) |           |          |
 summary      | character varying(255) |           |          |
Indexes:
    "movies_pkey" PRIMARY KEY, btree (id)

postgres=# \d movie_details

                        Table "public.movie_details"
      Column      |          Type          | Collation | Nullable | Default
------------------+------------------------+-----------+----------+---------
 name             | character varying(255) |           |          |
 director         | character varying(255) |           |          |
 production_house | character varying(255) |           |          |
```


### 2. Function ```query_vector_store()```

Query the vector store with natural language prompt(string) not necssarily matching the data. This will fetch the related records from the store.

```sql
postgres=# SELECT query_vector_store(service => 'ada',
                                     key => '<OpenAI auth key>',
                                     store => 'movie_lake_1',
                                     prompt => 'movie on hulk');

INFO:
 Query store meta data ( name  release_year  genre  summary  director  production_house )
                                   query_vector_store
----------------------------------------------------------
 ("Avengers: Endgame",2019,Action,"After the devastating events of Avengers: Infinity War, the universe is in ruins. With the help of remaining allies, the Avengers assemble once more to undo Thanos' actions and restore order to the universe.","Anthony Russo and Joe Russo","Walt Disney Studios Motion Pictures")
(1 row)

postgres=# SELECT query_vector_store(service => 'ada',
                                     key => '<OpenAI auth key>',
                                     store => 'movie_lake_1',
                                     prompt => 'ship hitting iceberg');

INFO:
 Query store meta data ( name  release_year  genre  summary  director  production_house )
                                   query_vector_store
----------------------------------------------------------
 (Titanic,1997,Romance,"A seventeen-year-old aristocrat falls in love with a kind but poor artist aboard the luxurious, ill-fated R.M.S. Titanic.","James Cameron","20th Century Fox")
(1 row)

postgres=# SELECT query_vector_store(service => 'ada',
                                     key => '<OpenAI auth key>',
                                     store => 'movie_lake_1',
                                     prompt => 'space travels',
                                     count => 2,
                                     similarity_algorithm => 'cosine_similarity');

INFO:
 Query store meta data ( name  release_year  genre  summary  director  production_house )
                                   query_vector_store
----------------------------------------------------------
 (Interstellar,2014,"Science Fiction","A team of explorers travel through a wormhole in space in an attempt to ensure humanity's survival.","Christopher Nolan","Warner Bros.")
 (Avatar,2009,Adventure,"A paraplegic marine dispatched to the moon Pandora on a unique mission becomes torn between following his orders and protecting the world he feels is his home.","James Cameron"," 20th Century Fox")
(2 rows)
```
The vector store is a regular postgres table defined by the SQL query result set and the embeddings. Drop just like a regular table after use.
```sql
DROP TABLE movie_lake_1;
```

### 3. Function ```get_insight()```

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
postgres=# SELECT entry, get_insight(service => 'chatgpt',
                                     key => '<OpenAI auth key>',
                                     textcolumn => entry,
                                     prompt => 'Get a single line description of:') AS Insight
              FROM information WHERE id > 0 AND id < 5;

       entry       |                                                                     insight
-------------------+--------------------------------------------------------------------------------------------------------------------------------------------------
 System R          |   System R is an IBM database system research project, which initiated the development of relational database technologies.
 Dihydrogen Oxide  |   Dihydrogen Oxide is another name for Water.
 TimescaleDB       |   TimescaleDB is an open-source relational database built for analyzing time-series data with the power and convenience of SQL.
 webbtelescope.org |   webbtelescope.org is a website providing information about the James Webb Space Telescope, an international space telescope launching in 2021.
(4 rows)
```
To generate a picture using Dall-E2

```sql
postgres=# SELECT entry, get_insight(service => 'dall-e2',
                                     key => '<OpenAI auth key>',
                                     textcolumn => entry,
                                     prompt => 'Make a picture of this:') AS pic
              FROM information WHERE id=2;

                                pic
--------------------------------------------------------------------
 https://***leapiprodscus.blob.core.****.net/private/org-fTI20YbDph2gXaSUgf3EV*** ...
(1 row)
```


### 4. Function ```get_insight_agg()```
Table 'chat' has the following data in the 'message' column

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
postgres=# SELECT get_insight_agg('chatgpt',
                                  '<OpenAI auth key>',
                                   message,
                                   'Choose a topic for the words:') AS "Topic"
              FROM chat WHERE ts::date = date '2023-01-01';

                        Topic
------------------------------------------------------
   "Maintenance and Performance of Four-Wheel Drives"
(1 row)
```

To generate a picture based on the above data using Dall-E2

```sql
postgres=# SELECT get_insight_agg('dall-e2',
                                  '<OpenAI auth key>',
                                   message,
                                   'Make a picture with the following:') AS "Topic"
              FROM chat WHERE ts::date = date '2023-01-01';

                                pic
--------------------------------------------------------------------
 https://***leapiprodscus.blob.core.****.net/private/org-fTI20YbDph2gXaSUgf3EV*** ...
(1 row)
```

### Help
```sql
SELECT pg_ai_help();
```

```sql
DROP EXTENSION pg_ai;
```

## Notes

Currently supported service models.

1. OpenAI - ChatGPT
2. OpenAI - Dall-E2
3. OpenAI - ADA(embeddings)

## TODOs

* Incorporate integration with additional local and remote LLMs.
* Implement parallelization and background loading of embedding vectors.
* Enhance support for curl read callbacks.
* Tokenization mechanism for larger data.
* Expand compatibility for other vector similarity algorithms.

## Disclaimer

This is not a official extension. All trademarks and copyrights are
the property of their respective owners.

The response received from the AI service is not interpreted or
modified by the extension, it is presented as received.