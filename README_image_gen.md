
## Text to image generation

pg_ai extension can be used to generate pictures from the column text.

## Getting Started

```sql
CREATE EXTENSION pg_ai;
```

```sql
SET pg_ai.service = 'OpenAI'(default);
```

Set the OpenAI [API Key](https://platform.openai.com/api-keys)
```sql
SET pg_ai.api_key='sk-********q';
```

Get the column data interpreted by LLM.
```sql
SELECT col1, pg_ai_generate_image(col1, NULL) AS "Pic" FROM information WHERE id > 1 AND id < 3;
```

Aggregate version of the above function.
```sql
SELECT pg_ai_generate_image_agg(col1, NULL) AS "Collage Pic" FROM chat WHERE ts::date = date '2023-01-01';
```
