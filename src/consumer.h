/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__

#include "ring_buffer.h"
#include "packet.h"

/**
 * @brief Consumer context structure used to manage synchronization and
 * file writing for each consumer thread.
 *
 * This structure contains all the necessary information for a consumer thread to
 * access the producer's buffer and write processed packets to a file. It also includes
 * mutexes and condition variables to ensure proper synchronization of access to shared
 * resources and to maintain the correct order of packets based on timestamps.
 */
typedef struct so_consumer_ctx_t
{
    /**
     * @brief Pointer to the producer's ring buffer.
     *
     * This is a reference to the producer's ring buffer from which the consumer will
     * dequeue data packets for processing.
     */
    struct so_ring_buffer_t *producer_rb;

    /**
     * @brief Output file name.
     *
     * This is the file where the processed packets will be written by the consumer.
     */
    const char *out_filename;

    /**
     * @brief Array for storing timestamps.
     *
     * This is an array of timestamps that are used to order the packets dequeued
     * from the producer's buffer. Each timestamp corresponds to a data packet.
     */
    unsigned long *times;

    /**
     * @brief Index for managing the timestamps array.
     *
     * This is a pointer to an index that is updated as timestamps are processed, helping
     * in managing the order of timestamps and facilitating proper synchronization of data.
     */
    unsigned long *idx;

    /**
     * @brief Mutex for protecting shared resources, especially the producer's ring buffer.
     *
     * This mutex ensures thread-safe access to shared resources, preventing race conditions
     * when accessing or modifying the producer's ring buffer and other shared data.
     */
    pthread_mutex_t mutex;

    /**
     * @brief Condition variable for synchronizing threads based on timestamp ordering.
     *
     * This condition variable is used to synchronize threads so that packets are processed
     * in the correct order according to their timestamps.
     */
    pthread_cond_t cond;

    /**
     * @brief Mutex for synchronizing file access while writing processed packets.
     *
     * This mutex ensures that only one consumer thread writes to the output file at a time,
     * preventing race conditions during file writes.
     */
    pthread_mutex_t file_mutex;
} so_consumer_ctx_t;

/**
 * Represents a consumer thread function that processes packets from a producer's ring buffer,
 * formats them, and writes them to a file in timestamp order.
 *
 * <p>This function operates in a multithreaded environment where multiple consumer threads
 * may access shared resources (e.g., ring buffer, output file). Synchronization is handled
 * using mutexes and condition variables to ensure thread-safe operations.</p>
 *
 * @param ctx The consumer context containing:
 *            <ul>
 *                <li>A reference to the producer's ring buffer.</li>
 *                <li>The name of the output file where processed packets are written.</li>
 *                <li>Synchronization primitives (mutexes and condition variables).</li>
 *                <li>Shared resources for timestamp ordering and output coordination.</li>
 *            </ul>
 *
 * <p>The function:
 * <ol>
 *     <li>Waits for packets to become available in the producer's ring buffer.</li>
 *     <li>Dequeues packets from the buffer and processes them.</li>
 *     <li>Synchronizes access to shared resources to maintain proper ordering of packets.</li>
 *     <li>Writes processed and formatted packet data to the output file.</li>
 * </ol>
 * </p>
 *
 * <p>Thread safety is achieved using the following mechanisms:
 * <ul>
 *     <li>Mutexes for exclusive access to shared resources (ring buffer and file).</li>
 *     <li>Condition variables to coordinate waiting and signaling between threads.</li>
 * </ul>
 * </p>
 *
 * <p>If the producer signals termination and the ring buffer is empty, the thread will exit.</p>
 *
 * @note Proper initialization of the consumer context is required before invoking this function.
 *       The output file is opened in append mode and closed when processing is complete.
 */
void consumer_thread(so_consumer_ctx_t *ctx);

/**
 * Wrapper function to execute the consumer thread in a `pthread` context.
 *
 * <p>This function serves as a bridge between the pthread library and the consumer thread function,
 * allowing the `pthread_create` function to start the consumer thread.</p>
 *
 * @param arg A pointer to the consumer context (`so_consumer_ctx_t`) passed to the actual
 *            `consumer_thread` function.
 * @return Always returns `NULL` since it does not provide a specific return value for the thread.
 *
 * @see consumer_thread
 */
void *consumer_wrapper(void *arg);

/**
 * Creates multiple consumer threads that process data from a shared producer's ring buffer
 * and write the processed results to an output file.
 *
 * <p>This function initializes the consumer context, allocates necessary resources,
 * and spawns the specified number of consumer threads. Each thread operates independently
 * but shares the same context for synchronization and data processing.</p>
 *
 * @param tids An array of `pthread_t` elements where the thread identifiers for the created
 *             threads will be stored. The array should have a size of at least `num_consumers`.
 * @param num_consumers The number of consumer threads to create.
 * @param rb A pointer to the shared ring buffer (`so_ring_buffer_t`) used by the producer
 *           and consumers for exchanging data.
 * @param out_filename The name of the file where the consumers will write the processed data.
 *
 * @return The number of consumer threads successfully created, or `-1` if an error occurs
 *         (e.g., memory allocation failure or thread creation failure).
 *
 * <h3>Details:</h3>
 * <ul>
 *   <li><b>Memory Management:</b> Allocates memory for the consumer context (`so_consumer_ctx_t`) 
 *       and shared resources (`ctx->times`, `ctx->idx`). These should be properly released after use.</li>
 *   <li><b>Synchronization:</b> Initializes mutexes and condition variables for thread-safe
 *       coordination and shared resource access.</li>
 *   <li><b>Thread Creation:</b> Uses `pthread_create` to start each consumer thread. Each thread
 *       executes the `consumer_wrapper` function, passing the shared consumer context as an argument.</li>
 * </ul>
 *
 * @note If any thread creation or memory allocation fails, the function returns immediately,
 *       leaving partially created threads and resources. Proper cleanup is necessary to avoid
 *       memory leaks or dangling threads.
 *
 * @see pthread_create, pthread_mutex_init, pthread_cond_init
 */
int create_consumers(pthread_t *tids,
                     int num_consumers,
                     so_ring_buffer_t *rb,
                     const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
