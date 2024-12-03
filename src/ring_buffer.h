// SPDX-License-Identifier: BSD-3-Clause

#ifndef __SO_RINGBUFFER_H__
#define __SO_RINGBUFFER_H__

#include <sys/types.h>
#include <string.h>
#include <pthread.h>

/**
 * @brief Structure defining a circular buffer.
 *
 * This structure manages a circular buffer used for storing and manipulating data in
 * a thread-safe manner, allowing concurrent access by producers and consumers. The buffer
 * is protected by mutexes and condition variables to ensure proper synchronization.
 */
typedef struct so_ring_buffer_t
{
    /**
     * @brief Pointer to the data buffer.
     *
     * This is an array of characters that stores the actual data written by the producer
     * and read by the consumer.
     */
    char *data;

    /**
     * @brief Read position in the buffer.
     *
     * This indicates where the next read operation will start.
     */
    size_t read_pos;

    /**
     * @brief Write position in the buffer.
     *
     * This indicates where the next write operation will start.
     */
    size_t write_pos;

    /**
     * @brief Current length of the buffer.
     *
     * This represents the amount of data currently stored in the buffer.
     */
    size_t len;

    /**
     * @brief Maximum capacity of the buffer.
     *
     * This is the total capacity of the buffer and represents the upper limit of how
     * much data can be stored.
     */
    size_t cap;

    /**
     * @brief Flag indicating whether the buffer should stop accepting new data.
     *
     * When this value is set to 1, the buffer will stop accepting new data, signaling
     * that the producer or consumer should stop processing.
     */
    int stop;

    /**
     * @brief Mutex for protecting the buffer.
     *
     * The mutex ensures exclusive access to the buffer during read and write operations
     * to prevent race conditions.
     */
    pthread_mutex_t mutex;

    /**
     * @brief Condition variable to signal when the buffer is not empty.
     *
     * Consumers wait on this condition variable if the buffer is empty.
     */
    pthread_cond_t not_empty;

    /**
     * @brief Condition variable to signal when the buffer is not full.
     *
     * Producers wait on this condition variable if the buffer is full.
     */
    pthread_cond_t not_full;
} so_ring_buffer_t;

/**
 * @brief Initializes a circular buffer.
 *
 * This function allocates memory for the buffer and sets the initial properties of the
 * buffer, including its capacity, read position, write position, and other synchronization
 * primitives.
 *
 * @param rb Pointer to the circular buffer structure.
 * @param cap The maximum capacity of the buffer (in bytes).
 * @return 0 if the initialization was successful, -1 if memory allocation failed.
 */
int ring_buffer_init(so_ring_buffer_t *rb, size_t cap);

/**
 * @brief Enqueues data into the circular buffer.
 *
 * This function blocks the thread until there is enough space in the buffer to add the
 * data. After the data is added, the buffer is updated, and the consumer is signaled that
 * data is available.
 *
 * @param rb Pointer to the circular buffer.
 * @param data Pointer to the data to be enqueued.
 * @param size The size (in bytes) of the data to be enqueued.
 * @return The size of the data enqueued (same as the `size` argument).
 */
ssize_t ring_buffer_enqueue(so_ring_buffer_t *rb, void *data, size_t size);

/**
 * @brief Dequeues data from the circular buffer.
 *
 * This function blocks the thread until data is available in the buffer to be dequeued.
 * After the data is dequeued, the buffer is updated, and the producer is signaled that
 * space is available.
 *
 * @param rb Pointer to the circular buffer.
 * @param data Pointer to the buffer where the dequeued data will be stored.
 * @param size The size (in bytes) of the data to be dequeued.
 * @return The size of the data dequeued (same as the `size` argument).
 */
ssize_t ring_buffer_dequeue(so_ring_buffer_t *rb, void *data, size_t size);

/**
 * @brief Destroys a circular buffer and releases resources.
 *
 * This function frees the memory allocated for the buffer's data and destroys the mutex
 * and condition variables used for synchronization.
 *
 * @param rb Pointer to the circular buffer to be destroyed.
 */
void ring_buffer_destroy(so_ring_buffer_t *rb);

/**
 * @brief Stops the buffer and signals all waiting threads.
 *
 * This function sets the `stop` flag to indicate that the buffer should stop and
 * broadcasts to notify all waiting threads that the buffer is stopped and they should
 * terminate gracefully.
 *
 * @param rb Pointer to the circular buffer to be stopped.
 */
void ring_buffer_stop(so_ring_buffer_t *rb);

#endif /* __SO_RINGBUFFER_H__ */
