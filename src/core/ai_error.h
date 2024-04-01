#define PG_AI_ERR_UNSUPPORTED_SERVICE "Unsupported service."
#define PG_AI_ERR_INVALID_OPTIONS "Invalid options to function."
#define PG_AI_ERR_INT_DATA_ERR "Internal error: cannot set data."
#define PG_AI_ERR_INT_PREP_TNSFR "Internal error: cannot prepare transfer call."
#define PG_AI_ERR_INT_TNSFR "Internal error: cannot transfer call."
#define PG_AI_ERR_NULL_STR "Null"
#define PG_AI_ERR_ARG_NULL "Argument is null."
#define PG_AI_ERR_TRANSFER_FAIL "Transfer failed. Try again."
#define PG_AI_ERR_DATA_TOO_BIG "Data to big, model only supports %lu words."

#define GET_ERR_TEXT(err) cstring_to_text(PG_AI_ERR_##err)
#define GET_ERR_STR(err) PG_AI_ERR_##err