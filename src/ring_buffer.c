/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdlib.h>
#include "ring_buffer.h"

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
    // Allocate memory for the buffer data array.
    ring->data = (char *)malloc(cap);
    if (!ring->data)
        return -1;  // Memory allocation failed.

    // Initialize ring buffer properties.
    ring->stop = 0;  // The buffer is not stopped at the beginning.
    ring->read_pos = 0;  // Read position starts at the beginning of the buffer.
    ring->write_pos = 0; // Write position starts at the beginning.
    ring->len = 0;       // The buffer is empty initially.
    ring->cap = cap;     // Set the buffer capacity.

    // Initialize synchronization primitives for thread-safe operations.
    pthread_mutex_init(&ring->mutex, NULL);  // Mutex for protecting the buffer's integrity.
    pthread_cond_init(&ring->not_empty, NULL); // Condition variable to signal when buffer has data.
    pthread_cond_init(&ring->not_full, NULL);  // Condition variable to signal when buffer has space.

    return 0;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
    pthread_mutex_lock(&ring->mutex); // Lock the mutex to ensure thread safety.

    // Wait until there is enough space in the buffer.
    while (ring->len + size > ring->cap)
        pthread_cond_wait(&ring->not_full, &ring->mutex);

    // Copy the data into the buffer at the current write position.
    memcpy(ring->data + ring->write_pos, data, size);

    // Update the write position and wrap it around if necessary.
    ring->write_pos = (ring->write_pos + size) % ring->cap;
    // Increase the length of the buffer by the size of the data.
    ring->len += size;

    pthread_mutex_unlock(&ring->mutex); // Unlock the mutex after the operation.
    pthread_cond_signal(&ring->not_empty); // Signal that the buffer is no longer empty.

    return size;  // Return the size of data enqueued.
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
    pthread_mutex_lock(&ring->mutex); // Lock the mutex for thread safety.

    // Copy the data from the buffer at the current read position.
    memcpy(data, ring->data + ring->read_pos, size);

    // Update the read position and wrap it around if necessary.
    ring->read_pos = (ring->read_pos + size) % ring->cap;
    // Decrease the length of the buffer by the size of the data.
    ring->len -= size;

    pthread_mutex_unlock(&ring->mutex); // Unlock the mutex after the operation.
    pthread_cond_signal(&ring->not_full); // Signal that the buffer is no longer full.

    return size;  // Return the size of data dequeued.
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
    free(ring->data);  // Free the memory allocated for the buffer data.
    // Destroy synchronization primitives.
    pthread_mutex_destroy(&ring->mutex);
    pthread_cond_destroy(&ring->not_empty);
    pthread_cond_destroy(&ring->not_full);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
    ring->stop = 1;  // Set the stop flag to indicate the buffer should stop.
    // Broadcast to notify all threads that the buffer is stopped.
    pthread_cond_broadcast(&ring->not_empty);
    pthread_cond_broadcast(&ring->not_full);
}
