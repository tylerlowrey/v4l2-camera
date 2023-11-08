#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "camera.h"

int main() {
    int camera_fd = setup_camera("/dev/video0", 800, 600);\

    if (camera_fd == -1) {
        perror("Error occurred while setting up camera");
        exit(EXIT_FAILURE);
    }

    struct v4l2_requestbuffers* request_buffers = request_mmap_buffers(camera_fd, 4);

    if (request_buffers == NULL) {
        perror("Error while requesting mmap buffers");
        exit(EXIT_FAILURE);
    }

    printf("request_mmap_buffers returned %d buffers\n", request_buffers->count);

    struct buffer* buffers = map_buffers(camera_fd, request_buffers);


    printf("Starting camera stream\n");
    if (start_stream(camera_fd) == -1) {
        perror("Error while starting camera stream");
        exit(EXIT_FAILURE);
    }

    sleep(10);

    if (stop_stream(camera_fd) == -1) {
        printf("Error while stopping camera stream");
        exit(EXIT_FAILURE);
    }

    cleanup_buffers(buffers, request_buffers->count);
    free(request_buffers);
    close(camera_fd);

    return 0;
}
