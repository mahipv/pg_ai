
## Text Modertations

pg_ai extension can be used to get moderations for the data in the tables. This model can classify data as potentially harmful across several categories.

## Getting Started

```sql
CREATE EXTENSION pg_ai;
```

Set the OpenAI [API Key](https://platform.openai.com/api-keys)
```sql
SET pg_ai.api_key='sk-********q';
```

Get the moderations for the column data.
```sql
SELECT col1, pg_ai_moderation(col1) FROM messages_table WHERE id=1;
```

Aggregate version of the above function.
```sql
SELECT pg_ai_moderation_agg(col1, NULL) AS "Topic" FROM messages_table WHERE id<10;
```