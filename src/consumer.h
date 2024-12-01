/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__

#include "ring_buffer.h"
#include "packet.h"

/* Consumer context structure used to manage synchronization and
 * file writing for each consumer thread.
 */
typedef struct so_consumer_ctx_t
{
    struct so_ring_buffer_t *producer_rb;  // Pointer to the producer's ring buffer from which packets are dequeued
    const char *out_filename;              // Output filename where the processed packets are written
    unsigned long *times, *idx;           // Array for storing timestamps and an index for managing the array
    /* Synchronization primitives to ensure proper ordering of packets based on timestamps */
    pthread_mutex_t mutex;                // Mutex to protect shared resources, especially the producer's ring buffer
    pthread_cond_t cond;                  // Condition variable to synchronize threads based on timestamp ordering
    pthread_mutex_t file_mutex;           // Mutex for synchronizing file access while writing processed packets
} so_consumer_ctx_t;

/* Function to create consumer threads.
 * Arguments:
 *   - tids: Array of thread IDs for consumer threads.
 *   - num_consumers: Number of consumer threads to create.
 *   - rb: Pointer to the producer's ring buffer.
 *   - out_filename: Name of the output file to write processed packets to.
 * Returns:
 *   - The number of created consumer threads on success.
 *   - -1 if an error occurs during thread creation.
 */
int create_consumers(pthread_t *tids,
                     int num_consumers,
                     so_ring_buffer_t *rb,
                     const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
