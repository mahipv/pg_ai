#ifndef PG_AI_THREAD_MANAGER_H
#define PG_AI_THREAD_MANAGER_H

#include <pthread.h>

#define NUM_PGAI_THREADS 1
#define NUM_LLM_THREADS 1
#define NUM_DB_THREADS 1
#define THREAD_QUEUE_LENGTH 1

void setup_pg_ai_threads(const size_t queue_length, const size_t num_producers,
						 const size_t num_workers, const size_t num_consumers,
						 void *service);

#endif /* PG_AI_THREAD_MANAGER_H */