#ifndef PG_AI_EMBED_THREADS_H
#define PG_AI_EMBED_THREADS_H

#include "pg_ai_threads.h"

extern bool embeddings_done;

void *pgai_worker_func(void *t_args);
void *llm_worker_func(void *t_args);
void *db_worker_func(void *t_args);
void embeddings_initialize_queue(PgAiThreadQueue *queue,
								 const AIService *ai_service);
void put_src_data(AIService *ai_service);
#endif /*PG_AI_EMBED_THREADS_H*/