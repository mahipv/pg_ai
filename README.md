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
get_insight('<service>', <column>, '<options file>')

get_insight_agg('<service>', <column>, '<options file>')
```

## Build & Installation

- pg_ai uses libcurl for REST communication, make sure the curl library is installed.
- check that pg_config is installed and available in the system's PATH.
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

### 1. Function ```get_insight()```

Say there is a table 'information' and has data, which needs re-interpretation
```sql
postgres=# \d information
                            Table "public.information"
 Column |  Type   | Collation | Nullable |                 Default
--------+---------+-----------+----------+-----------------------------------------
 id     | integer |           | not null | nextval('information_id_seq'::regclass)
 entry  | text    |           |          |
Indexes:
    "information_pkey" PRIMARY KEY, btree (id)
```

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

### 2. Function ```get_insight_agg()```

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
         "key":"<API Authorization KEY from your OpenAI account>",
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

Currently supported LLMs.

1. ChatGPT
2. Dall-E2

## TODOs
* Support to choose model within the AI service.
* Error reporting/reponse to be streamlined.
* Better PG context management(caching for faster REST calls).
* Support curl read callbacks for large data transfers.
* Add test suite.
* i18n/uncode, data translation service support.
* Support parallel calls within/across AI services.
* Support for AWS comprehend/bard/OpenAI wishper/+.
* Context analysis of multi-data with chatgpt-3.5-turbo.

## Disclaimer

This is not a official extension. All trademarks and copyrights are
the property of their respective owners.

The response received from the AI service is not interpreted or
modified by the extension, it is presented as received.