#define PG_AI_ERR_UNSUPPORTED_SERVICE "Unsupported service."
#define PG_AI_ERR_INVALID_OPTIONS "Invalid options to function."
#define PG_AI_ERR_INT_DATA_ERR "Internal error: cannot set data."
#define PG_AI_ERR_INT_PREP_TNSFR "Internal error: cannot prepare transfer call."
#define PG_AI_ERR_INT_TNSFR "Internal error: cannot transfer call."

#define GET_ERR_TEXT(err) cstring_to_text(PG_AI_ERR_##err)