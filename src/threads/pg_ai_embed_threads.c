#include "pg_ai_embed_threads.h"

#include "rest/rest_transfer.h"
#include "core/utils_pg_ai.h"

bool embeddings_done;
extern PgAiThreadQueue *src_queue;

static bool check_if_done()
{
	// this should check if the embeddings are done
	return embeddings_done;
}

static void put_data_to_queue(PgAiThreadQueue *queue, void *data)
{
	pthread_t tid = pthread_self();
	ereport(INFO, (errmsg("%lu Putting data %p into %s", tid, data,
						  queue->queue_name)));
	pthread_mutex_lock(&queue->mutex_data_array);

	/* wait for the consumers if the array is full */
	while (queue->data_array_count >= queue->data_array_size)
		pthread_cond_wait(&queue->less_data_array, &queue->mutex_data_array);

	assert(queue->data_array_count < queue->data_array_size);

	/* we are under lock now put in the next row */
	queue->data_array[queue->next_in++] = data;

	/* correct the index for next in */
	queue->next_in %= queue->data_array_size;

	/* increment the total */
	queue->data_array_count++;

	/* signal the consumer */
	pthread_cond_signal(&queue->more_data_array);

	/* we are done with the shared resource */
	pthread_mutex_unlock(&queue->mutex_data_array);
	ereport(INFO, (errmsg("%lu Putting data %p into %s - Complete", tid, data,
						  queue->queue_name)));
}

static void *get_data_from_queue(PgAiThreadQueue *queue)
{
	void *data;

	pthread_t tid = pthread_self();
	ereport(INFO,
			(errmsg("%lu Getting data from  %s", tid, queue->queue_name)));
	pthread_mutex_lock(&queue->mutex_data_array);

	/* wait for the producer if the array is empty */
	while (queue->data_array_count <= 0)
		pthread_cond_wait(&queue->more_data_array, &queue->mutex_data_array);

	assert(queue->data_array_count > 0);

	/* we are under lock now get the next row */
	data = queue->data_array[queue->next_out++];

	/* correct the index for next out */
	queue->next_out %= queue->data_array_size;

	/* decrement the total */
	queue->data_array_count--;

	/* signal the producer */
	pthread_cond_signal(&queue->less_data_array);

	/* we are done with the shared resource */
	pthread_mutex_unlock(&queue->mutex_data_array);

	ereport(INFO, (errmsg("%lu Getting data from  %s - Compelete(%p)", tid,
						  queue->queue_name, data)));

	return data;
}

void put_src_data(AIService *ai_service)
{
	put_data_to_queue(src_queue, ai_service);
}

static void get_src_data(AIService *ai_service)
{
	PgAiThreadQueue *queue = src_queue;
	AIService *src_ai_service;
	pthread_t tid = pthread_self();

	ereport(INFO, (errmsg("%lu Getting data from %s", tid, queue->queue_name)));
	pthread_mutex_lock(&queue->mutex_data_array);

	/* wait for the producer if the array is empty */
	while (queue->data_array_count <= 0)
		pthread_cond_wait(&queue->more_data_array, &queue->mutex_data_array);

	assert(queue->data_array_count > 0);

	/* we are under lock now get the next row */
	src_ai_service = queue->data_array[queue->next_out++];

	strncpy(ai_service->service_data->request_data,
			src_ai_service->service_data->request_data,
			ai_service->service_data->max_request_size);

	/* correct the index for next out */
	queue->next_out %= queue->data_array_size;

	/* decrement the total */
	queue->data_array_count--;

	/* signal the producer */
	pthread_cond_signal(&queue->less_data_array);

	/* we are done with the shared resource */
	pthread_mutex_unlock(&queue->mutex_data_array);
	ereport(INFO, (errmsg("%lu Getting data from %s - Complete", tid,
						  queue->queue_name)));
}

void *pgai_worker_func(void *t_args)
{
	bool done = false;
	void *ai_service = NULL;
	PgAiThreadArgs *args = (PgAiThreadArgs *)t_args;
	pthread_t tid = pthread_self();
	while (!done)
	{
		/* get data for embeddings */
		ai_service = get_data_from_queue(args->get_queue);

		/* get data from PG */
		get_src_data(ai_service);

		put_data_to_queue(args->put_queue, ai_service);

		/* check if we are done */
		done = check_if_done();
	}
	ereport(INFO, (errmsg("PgAi worker %lu complete", tid)));
	return NULL;
}

static void produce_data_for_embeddings(AIService *ai_service)
{
	/* this should call the rest_transfer to get the embeddings */
	init_rest_transfer(ai_service);
	ai_service->rest_response->data_size = 0;
	rest_transfer(ai_service);
	ereport(INFO,
			(errmsg("Data size %ld", ai_service->rest_response->data_size)));
	ai_service->service_data
		->response_data[ai_service->rest_response->data_size] = '\0';
	/*ereport(INFO, (errmsg("Producing data for embeddings %s",
						  ai_service->service_data->response_data))); */
}

void *llm_worker_func(void *t_args)
{
	bool done = false;
	void *ai_service;
	PgAiThreadArgs *args = (PgAiThreadArgs *)t_args;
	pthread_t tid = pthread_self();

	while (!done)
	{
		/* get data for embeddings */
		ai_service = get_data_from_queue(args->get_queue);

		/* generate the embeddings */
		produce_data_for_embeddings(ai_service);

		put_data_to_queue(args->put_queue, ai_service);

		/* check if we are done */
		done = check_if_done();
	}

	ereport(INFO, (errmsg("LLM worker %lu is complete", tid)));
	return NULL;
}

static void write_embeddings(AIService *ai_service)
{
	// this should write the embeddings to the store
	// ai_service->process_rest_response(ai_service);
	char query[1024];
	ereport(INFO, (errmsg("Before response process")));
	sprintf(query, "UPDATE ggmovies_vec_store_90s SET embeddings = '[1,23]'");
	SPI_execute(query, false, 0);
	ereport(INFO, (errmsg("After response process")));
}

void *db_worker_func(void *t_args)
{
	bool done = false;
	void *ai_service;
	PgAiThreadArgs *args = (PgAiThreadArgs *)t_args;
	pthread_t tid = pthread_self();
	while (!done)
	{
		/* get data for embeddings */
		ai_service = get_data_from_queue(args->get_queue);

		/* update the embeddings into the store */
		write_embeddings(ai_service);

		put_data_to_queue(args->put_queue, ai_service);

		/* check if we are done */
		done = check_if_done();
	}
	ereport(INFO, (errmsg("DB worker  %lu complete", tid)));
	return NULL;
}

void embeddings_initialize_queue(PgAiThreadQueue *queue,
								 const AIService *ai_service)
{
	for (int i = 0; i < queue->data_array_size; i++)
	{
		/* Make a copy of the existing service */
		AIService *new_ai_service = (AIService *)palloc0(sizeof(AIService));

		/* newly created service struct is same the single process version */
		memcpy(new_ai_service, ai_service, sizeof(AIService));

		new_ai_service->service_data = palloc0(sizeof(ServiceData));

		memcpy(new_ai_service->service_data, ai_service->service_data,
			   sizeof(ServiceData));

		/* allocate the request and response buffers accordingly */
		new_ai_service->service_data->request_data =
			palloc0(new_ai_service->service_data->max_request_size);
		new_ai_service->service_data->request_data[0] = '\0';

		new_ai_service->service_data->response_data =
			palloc0(new_ai_service->service_data->max_response_size);
		new_ai_service->service_data->response_data[0] = '\0';

		queue->data_array[i] = new_ai_service;
		queue->data_array_count++;
		ereport(INFO, (errmsg("Initializing %s : %d(%p)", queue->queue_name, i,
							  new_ai_service)));
	}
}
