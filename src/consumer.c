// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

// Consumer thread function to process packets and write them in order to a file
void consumer_thread(so_consumer_ctx_t *ctx)
{
    // Temporary storage for packet and output buffer
    so_packet_t packet;
    char out_buf[PKT_SZ];
    int len;

    // Open the output file for appending processed packets
    int fd = open(ctx->out_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("open");
        return;  // Error opening file, exit the consumer thread
    }

    while (1) {
        // Acquire the mutex to ensure safe access to shared resources (buffer)
        pthread_mutex_lock(&ctx->mutex);

        // Wait until there is data in the producer's ring buffer or the producer has stopped
        while (ctx->producer_rb->len == 0 && !ctx->producer_rb->stop)
            pthread_cond_wait(&ctx->producer_rb->not_empty, &ctx->mutex);

        // If the producer has stopped and no data is left in the buffer, terminate the thread
        if (ctx->producer_rb->stop && ctx->producer_rb->len == 0) {
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

        // Dequeue a packet from the producer's ring buffer
        ring_buffer_dequeue(ctx->producer_rb, &packet, sizeof(packet));

        // Protect the shared `times` array and update the timestamp for the current packet
        pthread_mutex_lock(&ctx->file_mutex);
        ctx->times[*ctx->idx] = packet.hdr.timestamp;  // Store the timestamp in the array
        (*ctx->idx)++;  // Increment the index to track the position
        pthread_mutex_unlock(&ctx->file_mutex);

        // Unlock the mutex since the packet has been safely dequeued
        pthread_mutex_unlock(&ctx->mutex);

        // Process the packet and prepare formatted output for writing
        int action = process_packet(&packet);  // Process the packet data
        unsigned long hash = packet_hash(&packet);  // Generate a hash for the packet
        unsigned long timestamp = packet.hdr.timestamp;  // Extract the timestamp

        // Format the packet data into the output buffer
        len = snprintf(out_buf, PKT_SZ, "%s %016lx %lu\n",
                       RES_TO_STR(action), hash, timestamp);

        // Lock the file mutex to ensure safe access to the output file
        pthread_mutex_lock(&ctx->file_mutex);

        // Wait until the current packet's timestamp matches the first timestamp in the array
        while (timestamp != ctx->times[0])
            pthread_cond_wait(&ctx->cond, &ctx->file_mutex);

        // Shift the timestamps array to make space for the next packet
        for (unsigned long i = 0; i < (*ctx->idx) - 1; i++)
            ctx->times[i] = ctx->times[i + 1];
        (*ctx->idx)--;  // Decrease the index after removing the processed timestamp

        // Broadcast any waiting threads that the data has been processed and written
        pthread_cond_broadcast(&ctx->cond);

        // Write the formatted packet data to the file
        write(fd, out_buf, len);

        // Unlock the file mutex after writing
        pthread_mutex_unlock(&ctx->file_mutex);
    }

    // Close the file once all packets are processed and written
    close(fd);
}

// Wrapper function to invoke the consumer thread
static void *consumer_wrapper(void *arg)
{
    consumer_thread((so_consumer_ctx_t *)arg);  // Call the actual consumer thread function
    return NULL;
}

// Function to create multiple consumer threads
int create_consumers(pthread_t *tids,
                     int num_consumers,
                     struct so_ring_buffer_t *rb,
                     const char *out_filename)
{
    // Allocate memory for the consumer context structure
    so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
    if (!ctx) return -1;  // Check if memory allocation failed

    // Initialize the consumer context with provided parameters
    ctx->producer_rb = rb;  // Set the producer's ring buffer
    ctx->out_filename = out_filename;  // Save the output filename for each consumer

    // Initialize mutexes and condition variables for synchronization
    pthread_mutex_init(&ctx->mutex, NULL);
    pthread_cond_init(&ctx->cond, NULL);
    pthread_mutex_init(&ctx->file_mutex, NULL);

    // Initialize shared resources used for timestamp ordering
    ctx->times = malloc(num_consumers * sizeof(unsigned long));  // Array to store timestamps
    ctx->idx = calloc(1, sizeof(unsigned long));  // Index to track the current position

    // Create the consumer threads
    for (int i = 0; i < num_consumers; i++) {
        if (pthread_create(&tids[i], NULL, consumer_wrapper, ctx) != 0) {
            perror("pthread_create");
            return -1;  // Error creating the thread, return failure
        }
    }

    // Return the number of consumer threads created
    return num_consumers;
}
