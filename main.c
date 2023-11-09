#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "camera.h"

void process_frame(struct buffer* buffer);

int main() {
    int camera_fd = setup_camera("/dev/video0", 800, 600);\

    if (camera_fd == -1) {
        perror("Error occurred while setting up camera");
        exit(EXIT_FAILURE);
    }

    struct v4l2_requestbuffers* request_buffers = request_mmap_buffers(camera_fd, 20);

    if (request_buffers == NULL) {
        perror("Error while requesting mmap buffers");
        exit(EXIT_FAILURE);
    }

    printf("request_mmap_buffers returned %d buffers\n", request_buffers->count);

    struct buffer* buffers = map_buffers(camera_fd, request_buffers);

    queue_buffers(camera_fd, request_buffers->count);

    printf("Starting camera stream\n");
    if (start_stream(camera_fd) == -1) {
        perror("Error while starting camera stream");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 1000; ++i) {
        int dequeued_buffer_index = dequeue_buffer(camera_fd);
        while (dequeued_buffer_index == -1) {
            printf("Received -1 response from de-queuing buffer\n");
            perror("");
            usleep(100000);
            dequeued_buffer_index = dequeue_buffer(camera_fd);
        }

        printf("Dequeued buffer with index: %d\n", dequeued_buffer_index);

        process_frame(&buffers[dequeued_buffer_index]);

        while (requeue_buffer(camera_fd, dequeued_buffer_index) == -1) {
            printf("WARN: Unable to requeue buffer with index %d\n", dequeued_buffer_index);
            usleep(100000);
        }

        printf("re-queued buffer with index %d\n", dequeued_buffer_index);

        usleep(100000);
    }

    if (stop_stream(camera_fd) == -1) {
        printf("Error while stopping camera stream");
        exit(EXIT_FAILURE);
    }

    cleanup_buffers(buffers, request_buffers->count);
    free(request_buffers);
    close(camera_fd);

    return 0;
}

void process_frame(struct buffer* buffer) {
    printf("Buffer info [Start address: %p, length: %zu]\n", buffer->start, buffer->length);
}