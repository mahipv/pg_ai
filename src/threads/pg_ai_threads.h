#ifndef PG_AI_THREADS_H
#define PG_AI_THREADS_H

#include <assert.h>
#include <pthread.h>

#include "core/ai_service.h"
#include "postgres.h"

typedef struct PgAiThreadQueue
{
	char queue_name[PG_AI_DESC_LENGTH];

	/* the row data is converted into string and passed on */
	void **data_array;

	/* optimize to get the size of the nos of buffers to process */
	size_t data_array_size;

	/* current count in the data_array */
	size_t data_array_count;

	/* the in  offset into the queue */
	size_t next_in;

	/* the out offset of the queue */
	size_t next_out;

	/* pthread variables to sync this struct */
	pthread_mutex_t mutex_data_array;
	pthread_cond_t more_data_array;
	pthread_cond_t less_data_array;

} PgAiThreadQueue;

typedef struct PgAiThreadArgs
{
	const char *thread_name; // name of the thread
	PgAiThreadQueue *get_queue;
	PgAiThreadQueue *put_queue;
} PgAiThreadArgs;

#endif // PG_AI_THREADS_H