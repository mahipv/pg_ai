CREATE OR REPLACE FUNCTION get_insights(
       ai_service              NAME,
       textcolumn	       TEXT,
       auth_key		       NAME = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'get_insights' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _get_insights_agg_transfn(
	state		internal,
	ai_service	NAME,
	val		anyelement,
	auth_key	NAME = NULL
)RETURNS internal AS 'MODULE_PATHNAME', 'get_insights_agg_transfn' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _get_insights_agg_finalfn(
	state		internal,
	ai_service	NAME,
	val		anyelement,
	auth_key	NAME = NULL
)RETURNS TEXT AS 'MODULE_PATHNAME', 'get_insights_agg_finalfn' LANGUAGE C IMMUTABLE;

DROP AGGREGATE IF EXISTS get_insights_agg (NAME, ANYELEMENT, NAME);
CREATE AGGREGATE get_insights_agg (NAME, ANYELEMENT, NAME)
(
    sfunc = _get_insights_agg_transfn,
    stype = internal,
    finalfunc = _get_insights_agg_finalfn,
    finalfunc_extra
);

CREATE OR REPLACE FUNCTION pg_ai_help()
RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_help' LANGUAGE C IMMUTABLE;
