CREATE OR REPLACE FUNCTION get_insight(
	ai_service	NAME,
	textcolumn	TEXT,
	param_file	NAME
)RETURNS TEXT AS 'MODULE_PATHNAME', 'get_insight' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _get_insight_agg_transfn(
	state		internal,
	ai_service	NAME,
	val		anyelement,
	param_file	NAME
)RETURNS internal AS 'MODULE_PATHNAME', 'get_insight_agg_transfn' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _get_insight_agg_finalfn(
	state		internal,
	ai_service	NAME,
	val		anyelement,
	param_file	NAME
)RETURNS TEXT AS 'MODULE_PATHNAME', 'get_insight_agg_finalfn' LANGUAGE C IMMUTABLE;

DROP AGGREGATE IF EXISTS get_insight_agg (NAME, ANYELEMENT, NAME);
CREATE AGGREGATE get_insight_agg (NAME, ANYELEMENT, NAME)
(
	sfunc = _get_insight_agg_transfn,
	stype = internal,
	finalfunc = _get_insight_agg_finalfn,
	finalfunc_extra
);

CREATE OR REPLACE FUNCTION pg_ai_help()
RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_help' LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION pg_ai_help_options()
RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_ai_help_options' LANGUAGE C IMMUTABLE;
