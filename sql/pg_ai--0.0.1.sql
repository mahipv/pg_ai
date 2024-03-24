/*
* Functions to get column insights from AI using gpt.
*/
CREATE OR REPLACE FUNCTION pg_ai_insight(
	column_name	TEXT,
	prompt      TEXT = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_insight' LANGUAGE C IMMUTABLE;

/*
* Aggregate version of the pg_ai_insight function.
*/
CREATE OR REPLACE FUNCTION _pg_ai_insight_agg_transfn(
	state		internal,
	column_name	TEXT,
	prompt      TEXT
)RETURNS internal AS 'MODULE_PATHNAME', 'pg_ai_insight_agg_transfn' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _pg_ai_insight_agg_finalfn(
	state		internal,
	column_name  	TEXT,
	prompt      TEXT
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_insight_agg_finalfn' LANGUAGE C IMMUTABLE;

CREATE AGGREGATE pg_ai_insight_agg (column_name TEXT, prompt TEXT)
(
	sfunc = _pg_ai_insight_agg_transfn,
	stype = internal,
	finalfunc = _pg_ai_insight_agg_finalfn,
	finalfunc_extra
);

/*
* Function to get help on the AI functions.
*/
CREATE OR REPLACE FUNCTION pg_ai_help()
RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_help' LANGUAGE C IMMUTABLE;

/*
* Function to create a vector store.
*/
CREATE OR REPLACE FUNCTION pg_ai_create_vector_store(
	store  			NAME,
	sql_query      	TEXT,
	notes		  	NAME = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_create_vector_store' LANGUAGE C IMMUTABLE;

/*
* Function to query the vector store.
*/
CREATE OR REPLACE FUNCTION pg_ai_query_vector_store(
	store		  			NAME,
	nl_query  				TEXT,
	count                   INT = 2
)RETURNS SETOF record AS 'MODULE_PATHNAME', 'pg_ai_query_vector_store' LANGUAGE C IMMUTABLE;

/*
* Function to generate the image from the column text.
*/
CREATE OR REPLACE FUNCTION pg_ai_generate_image(
	column_name		TEXT,
	prompt         	TEXT = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_generate_image' LANGUAGE C IMMUTABLE;


/*
* Aggregate version of the pg_ai_generate_image function.
*/
CREATE OR REPLACE FUNCTION _pg_ai_generate_image_agg_transfn(
	state		internal,
	column_name	TEXT,
	prompt      TEXT
)RETURNS internal AS 'MODULE_PATHNAME', 'pg_ai_generate_image_agg_transfn' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _pg_ai_generate_image_agg_finalfn(
	state		internal,
	column_name  	TEXT,
	prompt      TEXT
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_generate_image_agg_finalfn' LANGUAGE C IMMUTABLE;

CREATE AGGREGATE pg_ai_generate_image_agg (column_name TEXT, prompt TEXT)
(
	sfunc = _pg_ai_generate_image_agg_transfn,
	stype = internal,
	finalfunc = _pg_ai_generate_image_agg_finalfn,
	finalfunc_extra
);

/*
* Function to get the moderation of the text.
*/
CREATE OR REPLACE FUNCTION pg_ai_moderation(
	column_name		TEXT
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_moderation' LANGUAGE C IMMUTABLE;

/*
* Aggregate version of the pg_ai_moderation function.
*/
CREATE OR REPLACE FUNCTION _pg_ai_moderation_agg_transfn(
	state		internal,
	column_name 	TEXT
)RETURNS internal AS 'MODULE_PATHNAME', 'pg_ai_moderation_agg_transfn' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _pg_ai_moderation_agg_finalfn(
	state		internal,
	column_name  	TEXT
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_moderation_agg_finalfn' LANGUAGE C IMMUTABLE;

CREATE AGGREGATE pg_ai_moderation_agg (column_name TEXT)
(
	sfunc = _pg_ai_moderation_agg_transfn,
	stype = internal,
	finalfunc = _pg_ai_moderation_agg_finalfn,
	finalfunc_extra
);