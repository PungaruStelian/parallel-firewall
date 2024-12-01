// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

void consumer_thread(so_consumer_ctx_t *ctx)
{
    so_packet_t packet;
    char out_buf[PKT_SZ];
    int len;
    int fd = open(ctx->out_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }

    while (1) {
        pthread_mutex_lock(&ctx->mutex);
        while (ctx->producer_rb->len == 0 && !ctx->producer_rb->stop)
            pthread_cond_wait(&ctx->producer_rb->not_empty, &ctx->mutex);

        if (ctx->producer_rb->stop && ctx->producer_rb->len == 0) {
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

        ring_buffer_dequeue(ctx->producer_rb, &packet, sizeof(packet));
        pthread_mutex_unlock(&ctx->mutex);

        // Process packet and write formatted output
        int action = process_packet(&packet);
        unsigned long hash = packet_hash(&packet);
        unsigned long timestamp = packet.hdr.timestamp;

        len = snprintf(out_buf, PKT_SZ, "%s %016lx %lu\n",
            RES_TO_STR(action), hash, timestamp);

        pthread_mutex_lock(&ctx->file_mutex);
        write(fd, out_buf, len);
        pthread_mutex_unlock(&ctx->file_mutex);
    }

    close(fd);
}

static void *consumer_wrapper(void *arg)
{
    consumer_thread((so_consumer_ctx_t *)arg);
    return NULL;
}

int create_consumers(pthread_t *tids,
                     int num_consumers,
                     struct so_ring_buffer_t *rb,
                     const char *out_filename)
{
    so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
    ctx->producer_rb = rb;
    ctx->out_filename = out_filename;  // salvăm numele fișierului în context
    pthread_mutex_init(&ctx->mutex, NULL);
    pthread_cond_init(&ctx->cond, NULL);
    pthread_mutex_init(&ctx->file_mutex, NULL);
    ctx->producer_rb->stop = 0;

    for (int i = 0; i < num_consumers; i++) {
        if (pthread_create(&tids[i], NULL, consumer_wrapper, ctx) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    return num_consumers;
}
