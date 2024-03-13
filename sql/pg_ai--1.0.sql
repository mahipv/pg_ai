CREATE OR REPLACE FUNCTION pg_ai_insight(
	textcolumn		TEXT,
	prompt         	TEXT = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_insight' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _pg_ai_insight_agg_transfn(
	state		internal,
	textcolumn  TEXT,
	prompt      TEXT
)RETURNS internal AS 'MODULE_PATHNAME', 'pg_ai_insight_agg_transfn' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _pg_ai_insight_agg_finalfn(
	state		internal,
	textcolumn  TEXT,
	prompt      TEXT
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_insight_agg_finalfn' LANGUAGE C IMMUTABLE;

CREATE AGGREGATE pg_ai_insight_agg (textcolumn TEXT, prompt TEXT) 
(
	sfunc = _pg_ai_insight_agg_transfn,
	stype = internal,
	finalfunc = _pg_ai_insight_agg_finalfn,
	finalfunc_extra
);

CREATE OR REPLACE FUNCTION pg_ai_help()
RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_help' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION pg_ai_create_vector_store(
	store  			NAME,
	query       	TEXT,
	notes		  	NAME = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_create_vector_store' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION pg_ai_query_vector_store(
	store		  			NAME,
	prompt  				TEXT,
	count                   INT = 3
)RETURNS SETOF record AS 'MODULE_PATHNAME', 'pg_ai_query_vector_store' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION pg_ai_generate_image(
	textcolumn		TEXT,
	prompt         	TEXT = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_generate_image' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _pg_ai_generate_image_agg_transfn(
	state		internal,
	textcolumn  TEXT,
	prompt      TEXT
)RETURNS internal AS 'MODULE_PATHNAME', 'pg_ai_generate_image_agg_transfn' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _pg_ai_generate_image_agg_finalfn(
	state		internal,
	textcolumn  TEXT,
	prompt      TEXT
)RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_generate_image_agg_finalfn' LANGUAGE C IMMUTABLE;

CREATE AGGREGATE pg_ai_generate_image_agg (textcolumn TEXT, prompt TEXT) 
(
	sfunc = _pg_ai_generate_image_agg_transfn,
	stype = internal,
	finalfunc = _pg_ai_generate_image_agg_finalfn,
	finalfunc_extra
);