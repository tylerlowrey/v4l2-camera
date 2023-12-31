#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "camera.h"
#include "helper.h"
#include "apriltag_detection.h"

int FRAME_WIDTH = 800;
int FRAME_HEIGHT = 600;
int NUM_BUFFERS = 20;

int setup_socket(struct sockaddr_in* socket_address, char* server_ip, uint16_t server_port);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server address> <server port>\\n", argv[0]);
        return 1;
    }

    char* server_address = argv[1];
    uint16_t server_port = (uint16_t)atoi(argv[2]);

    struct sockaddr_in socket_address;
    int socket_fd = setup_socket(&socket_address, server_address, server_port);


    int camera_fd = setup_camera("/dev/video0", FRAME_WIDTH, FRAME_HEIGHT);

    if (camera_fd == -1) {
        perror("Error occurred while setting up camera");
        exit(EXIT_FAILURE);
    }

    struct v4l2_requestbuffers* request_buffers = request_mmap_buffers(camera_fd, NUM_BUFFERS);

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

    struct image_u8** grayscale_image_buffers = calloc(NUM_BUFFERS, sizeof(struct image_u8*));
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        grayscale_image_buffers[i] = image_u8_create(FRAME_WIDTH, FRAME_HEIGHT);
    }

    apriltag_family_t *apriltag_family = tag16h5_create();
    apriltag_detector_t *apriltag_detector = apriltag_detector_create();
    apriltag_detector_add_family(apriltag_detector, apriltag_family);

    char* filename = (char*) malloc(sizeof(char) * 128);
    const char* filename_format = "build/output_%d.ppm";
    unsigned char udp_data[2] = { 0, 0};

    for (int i = 0; i < 10000; ++i) {
        int buffer_index = dequeue_buffer(camera_fd);
        while (buffer_index == -1) {
            printf("Received -1 response from de-queuing buffer\n");
            usleep(100000);
            buffer_index = dequeue_buffer(camera_fd);
        }

#if DEBUG
        printf("Dequeued buffer with index: %d\n", buffer_index);
#endif
        prepare_frame_for_processing(&buffers[buffer_index], grayscale_image_buffers[buffer_index]);
        snprintf(filename, sizeof(char) * 128, filename_format, i % 20);

#if DEBUG
        write_grayscale_image_to_file(filename, grayscale_image_buffers[buffer_index]);
#endif

        int detected_apriltag_id = detect_april_tag(
        grayscale_image_buffers[buffer_index],
        apriltag_detector
        );

        if (detected_apriltag_id == -1) {
            printf("No april tag detected\n");
            udp_data[0] = 0;
            udp_data[1] = 0;
            sendto(socket_fd, udp_data, sizeof(udp_data), 0, (const struct sockaddr*) &socket_address, sizeof(socket_address));

        } else {
            printf("Detected april tag with ID: %d\n", detected_apriltag_id);
            udp_data[0] = detected_apriltag_id;
            udp_data[1] = 1;
            sendto(socket_fd, udp_data, sizeof(udp_data), 0, (const struct sockaddr*) &socket_address, sizeof(socket_address));
        }

        while (requeue_buffer(camera_fd, buffer_index) == -1) {
            printf("WARN: Unable to requeue buffer with index %d\n", buffer_index);
            usleep(100000);
        }

#if DEBUG
        printf("re-queued buffer with index %d\n", buffer_index);
#endif

        usleep(50000);
    }

    if (stop_stream(camera_fd) == -1) {
        printf("Error while stopping camera stream");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < request_buffers->count; ++i) {
        free(grayscale_image_buffers[i]);
    }
    free(grayscale_image_buffers);
    cleanup_buffers(buffers, request_buffers->count);
    free(request_buffers);
    close(camera_fd);

    return 0;
}

int setup_socket(struct sockaddr_in* socket_address, char* server_ip, uint16_t server_port) {
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return socket_fd;
    }

    memset(socket_address, 0, sizeof(*socket_address));
    socket_address->sin_family = AF_INET;
    socket_address->sin_port = htons(server_port);
    socket_address->sin_addr.s_addr = inet_addr(server_ip);
    return socket_fd;
}



