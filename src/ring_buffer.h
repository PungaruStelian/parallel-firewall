// SPDX-License-Identifier: BSD-3-Clause

#ifndef __SO_RINGBUFFER_H__
#define __SO_RINGBUFFER_H__

#include <sys/types.h>
#include <string.h>
#include <pthread.h>

typedef struct so_ring_buffer_t
{
	// Pointer to the data buffer where the actual content is stored
	// This buffer holds the data that is passed between producers and consumers
	char *data;

	// Position where the next read operation will begin
	// This is used to manage the read operations and ensure data is processed in the correct order
	size_t read_pos;

	// Position where the next write operation will begin
	// This is where data will be inserted into the buffer by the producer
	size_t write_pos;

	// Current length of the data in the buffer (how much data is stored)
	// This helps in tracking the number of valid entries in the buffer
	size_t len;

	// Capacity of the buffer (how much data it can hold in total)
	// This helps in determining when the buffer is full
	size_t cap;

	// Flag indicating if the buffer should stop accepting new data
	// This is used to signal when the producer or consumer should stop processing
	int stop;

	// Mutex to ensure that only one thread can access the buffer at a time
	// This prevents race conditions when accessing the buffer's data or managing positions
	pthread_mutex_t mutex;

	// Condition variable to signal when there is data available to read in the buffer
	// Consumers wait on this condition variable if the buffer is empty
	pthread_cond_t not_empty;

	// Condition variable to signal when there is space available to write in the buffer
	// Producers wait on this condition variable if the buffer is full
	pthread_cond_t not_full;
} so_ring_buffer_t;

/* Initializes a ring buffer.
 * Arguments:
 *   - ring: Pointer to the ring buffer structure that needs to be initialized.
 *   - cap: The maximum capacity of the buffer (in bytes).
 * Returns:
 *   - 0 on success (buffer initialized).
 *   - -1 if memory allocation for the buffer fails.
 */
int ring_buffer_init(so_ring_buffer_t *rb, size_t cap);

/* Enqueues data into the ring buffer.
 * Arguments:
 *   - ring: Pointer to the ring buffer.
 *   - data: Pointer to the data to be enqueued.
 *   - size: Size (in bytes) of the data to be enqueued.
 * Returns:
 *   - The size of the data enqueued (same as the size argument).
 *   - The function blocks if there is insufficient space in the buffer.
 */
ssize_t ring_buffer_enqueue(so_ring_buffer_t *rb, void *data, size_t size);

/* Dequeues data from the ring buffer.
 * Arguments:
 *   - ring: Pointer to the ring buffer.
 *   - data: Pointer to the buffer where the dequeued data will be stored.
 *   - size: Size (in bytes) of the data to be dequeued.
 * Returns:
 *   - The size of the data dequeued (same as the size argument).
 *   - The function blocks if the buffer is empty.
 */
ssize_t ring_buffer_dequeue(so_ring_buffer_t *rb, void *data, size_t size);

/* Destroys a ring buffer and releases all resources.
 * Arguments:
 *   - ring: Pointer to the ring buffer to be destroyed.
 */
void ring_buffer_destroy(so_ring_buffer_t *rb);

/* Stops the ring buffer and signals all threads waiting on conditions.
 * Arguments:
 *   - ring: Pointer to the ring buffer to be stopped.
 * This function sets the `stop` flag to 1 and broadcasts to notify all waiting threads
 * that the buffer is being stopped, allowing them to terminate gracefully.
 */
void ring_buffer_stop(so_ring_buffer_t *rb);

#endif /* __SO_RINGBUFFER_H__ */
