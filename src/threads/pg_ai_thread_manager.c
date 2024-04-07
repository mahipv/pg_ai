#include "pg_ai_thread_manager.h"

#include <unistd.h> /// for sleep

#include "pg_ai_threads.h"
#include "pg_ai_embed_threads.h"
#include "core/ai_config.h"

#define PGAI_QUEUE_STR "PgAi Queue"
#define LLM_QUEUE_STR "LLM Queue"
#define DB_QUEUE_STR "DB Queue"
#define SRC_QUEUE_STR "Src Queue"

pthread_t pgai_workers[NUM_PGAI_THREADS];
pthread_t llm_workers[NUM_LLM_THREADS];
pthread_t db_workers[NUM_DB_THREADS];
PgAiThreadQueue *pgai_queue, *llm_queue, *db_queue;
PgAiThreadQueue *src_queue;

static PgAiThreadQueue *allocate_queue(const size_t queue_length,
									   const char *name)
{
	PgAiThreadQueue *queue =
		(PgAiThreadQueue *)palloc0(sizeof(PgAiThreadQueue));
	strncpy(queue->queue_name, name, PG_AI_DESC_LENGTH);
	queue->data_array = (void **)palloc0(sizeof(void *) * queue_length);
	queue->data_array_size = queue_length;
	queue->data_array_count = 0;
	queue->next_in = 0;
	queue->next_out = 0;
	pthread_mutex_init(&queue->mutex_data_array, NULL);
	pthread_cond_init(&queue->more_data_array, NULL);
	pthread_cond_init(&queue->less_data_array, NULL);
	return queue;
}

void setup_pg_ai_threads(const size_t queue_length,
						 const size_t num_pgai_workers,
						 const size_t num_llm_workers,
						 const size_t num_db_workers, void *service)
{
	PgAiThreadArgs *pgai_worker_args;
	PgAiThreadArgs *llm_worker_args;
	PgAiThreadArgs *db_worker_args;

	/* get the free and work queues ready */
	pgai_queue = allocate_queue(queue_length, PGAI_QUEUE_STR);
	llm_queue = allocate_queue(queue_length, LLM_QUEUE_STR);
	db_queue = allocate_queue(queue_length, DB_QUEUE_STR);
	src_queue = allocate_queue(1, SRC_QUEUE_STR);

	embeddings_initialize_queue(pgai_queue, service);

	/* create the producer threads */
	pgai_worker_args = (PgAiThreadArgs *)palloc0(sizeof(PgAiThreadArgs));
	pgai_worker_args->get_queue = pgai_queue;
	pgai_worker_args->put_queue = llm_queue;
	for (int i = 0; i < num_pgai_workers; i++)
		pthread_create(&pgai_workers[i], NULL, pgai_worker_func,
					   pgai_worker_args);

	/* create the worker threads */
	llm_worker_args = (PgAiThreadArgs *)palloc0(sizeof(PgAiThreadArgs));
	llm_worker_args->get_queue = llm_queue;
	llm_worker_args->put_queue = db_queue;
	for (int i = 0; i < num_llm_workers; i++)
		pthread_create(&llm_workers[i], NULL, llm_worker_func, llm_worker_args);

	/* create the consumer threads */
	db_worker_args = (PgAiThreadArgs *)palloc0(sizeof(PgAiThreadArgs));
	db_worker_args->get_queue = db_queue;
	db_worker_args->put_queue = pgai_queue;
	for (int i = 0; i < num_db_workers; i++)
		pthread_create(&db_workers[i], NULL, db_worker_func, db_worker_args);

	sleep(3);
	ereport(INFO, (errmsg("After sleep")));
	// print the thread ids
	for (int i = 0; i < num_pgai_workers; i++)
		ereport(INFO, (errmsg("PgAi thread id %d: %lu", i, pgai_workers[i])));

	// print the thread ids
	for (int i = 0; i < num_llm_workers; i++)
		ereport(INFO, (errmsg("LLM thread id %d: %lu", i, llm_workers[i])));

	// print the thread ids
	for (int i = 0; i < num_db_workers; i++)
		ereport(INFO, (errmsg("DB thread id %d: %lu", i, db_workers[i])));
}
