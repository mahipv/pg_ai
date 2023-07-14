CREATE OR REPLACE FUNCTION get_insight(
	service	    NAME,
	key	 	   	TEXT,
	textcolumn	TEXT,
	prompt      TEXT
)RETURNS TEXT AS 'MODULE_PATHNAME', 'get_insight' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _get_insight_agg_transfn(
	state		internal,
	service	    NAME,
    key         TEXT,
	textcolumn  TEXT,
	prompt      TEXT
)RETURNS internal AS 'MODULE_PATHNAME', 'get_insight_agg_transfn' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _get_insight_agg_finalfn(
	state		internal,
	service	    NAME,
    key         TEXT,
	textcolumn  TEXT,
	prompt      TEXT
)RETURNS TEXT AS 'MODULE_PATHNAME', 'get_insight_agg_finalfn' LANGUAGE C IMMUTABLE;

DROP AGGREGATE IF EXISTS get_insight_agg (NAME, ANYELEMENT, NAME);
CREATE AGGREGATE get_insight_agg (service NAME, key TEXT, textcolumn TEXT, prompt TEXT)
(
	sfunc = _get_insight_agg_transfn,
	stype = internal,
	finalfunc = _get_insight_agg_finalfn,
	finalfunc_extra
);

CREATE OR REPLACE FUNCTION pg_ai_help()
RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_help' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION create_vector_store(
	service		NAME,
	key	 	   	TEXT,
	store  		NAME,
	query       TEXT,
	prompt  	NAME = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'create_vector_store' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION query_vector_store(
	service				    NAME,
	key						TEXT,
	store		  			NAME,
	prompt  				TEXT,
	count                   INT = 1,
	similarity_algorithm   	NAME = 'cosine_similarity'
)RETURNS SETOF record AS 'MODULE_PATHNAME', 'query_vector_store' LANGUAGE C IMMUTABLE;