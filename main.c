#include <stdio.h>
#include <stdlib.h>

#include "camera.h"

int main() {
    int camera_fd = setup_camera("/dev/video0", 800, 600);\

    if (camera_fd == -1) {
        printf("Error occurred while setting up camera");
        exit(EXIT_FAILURE);
    }

    struct v4l2_requestbuffers* request_buffers = request_mmap_buffers(camera_fd, 4);

    if (request_buffers == NULL) {
        printf("Error while requesting mmap buffers");
        exit(EXIT_FAILURE);
    }

    printf("request_mmap_buffers returned %d buffers", request_buffers->count);

    struct buffer* buffers = map_buffers(camera_fd, request_buffers);

    start_stream(camera_fd);

    return 0;
}
