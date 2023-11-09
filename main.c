#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "camera.h"
#include "helper.h"
#include "apriltag/apriltag.h"
#include "apriltag/tag16h5.h"
#include "apriltag/common/image_types.h"

int FRAME_WIDTH = 800;
int FRAME_HEIGHT = 600;

void process_frame(struct buffer* buffer, uint8_t* rgb_data_buffer);
int write_frame_to_file(const char* filename, uint8_t* rgb_data);
int convert_to_apriltag_image(const uint8_t* buffer, struct image_u8* apriltag_image);
int detect_apriltag(struct image_u8* apriltag_image, apriltag_detector_t* apriltag_detector);

int main() {
    int camera_fd = setup_camera("/dev/video0", FRAME_WIDTH, FRAME_HEIGHT);

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

    uint8_t** rgb_data_buffers = calloc(request_buffers->count, sizeof(uint8_t*));
    for (int i = 0; i < request_buffers->count; ++i) {
        rgb_data_buffers[i] = (uint8_t*) malloc(FRAME_WIDTH * FRAME_HEIGHT * 3);
    }

    char* filename = (char*) malloc(sizeof(char) * 128);
    const char* filename_format = "output_%d.ppm";

    struct image_u8* apriltag_image = image_u8_create(FRAME_WIDTH, FRAME_HEIGHT);
    apriltag_family_t *apriltag_family = tag16h5_create();
    apriltag_detector_t *apriltag_detector = apriltag_detector_create();
    apriltag_detector_add_family(apriltag_detector, apriltag_family);

    for (int i = 0; i < 10000; ++i) {
        int dequeued_buffer_index = dequeue_buffer(camera_fd);
        while (dequeued_buffer_index == -1) {
            printf("Received -1 response from de-queuing buffer\n");
            perror("");
            usleep(100000);
            dequeued_buffer_index = dequeue_buffer(camera_fd);
        }

        printf("Dequeued buffer with index: %d\n", dequeued_buffer_index);

        process_frame(&buffers[dequeued_buffer_index], rgb_data_buffers[dequeued_buffer_index]);
        snprintf(filename, sizeof(char) * 128, filename_format, i);
        convert_to_apriltag_image(rgb_data_buffers[dequeued_buffer_index], apriltag_image);
        int detected_apriltag_id = detect_apriltag(apriltag_image, apriltag_detector);

        if (detected_apriltag_id == -1) {
            printf("No april tag detected\n");
        } else {
            printf("Detected april tag with ID: %d\n", detected_apriltag_id);
        }

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

    for (int i = 0; i < request_buffers->count; ++i) {
        free(rgb_data_buffers[i]);
    }
    free(rgb_data_buffers);
    cleanup_buffers(buffers, request_buffers->count);
    free(request_buffers);
    close(camera_fd);

    return 0;
}

void process_frame(struct buffer* buffer, uint8_t* rgb_data_buffer) {
    uint8_t *yuyv_ptr = (uint8_t*)buffer->start;
    size_t yuyv_length = buffer->length;

    if (yuyv_length != FRAME_WIDTH * FRAME_HEIGHT * 2) {
        printf("Could not process frame because yuyv buffer length did not match expected length");
        return;
    }

    for (size_t i = 0; i < yuyv_length; i += 4) {
        yuyv_to_rgb(yuyv_ptr[i], yuyv_ptr[i + 1], yuyv_ptr[i + 2], yuyv_ptr[i + 3], &rgb_data_buffer[(i / 4) * 6]);
    }
}

int write_frame_to_file(const char* filename, uint8_t* rgb_data) {
    // Open the file for writing
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Unable to open file for writing");
        return -1;
    }

    // Write the PPM header
    // P6 is the magic number for PPM binary format
    // Then the image width and height and the maximum color value (255 for 8 bits per channel)
    fprintf(file, "P6\n%d %d\n255\n", FRAME_WIDTH, FRAME_HEIGHT);

    // Write the RGB data
    size_t written = fwrite(rgb_data, 1, FRAME_WIDTH * FRAME_HEIGHT * 3, file);
    if (written != FRAME_WIDTH * FRAME_HEIGHT * 3) {
        perror("Error writing RGB data to file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int convert_to_apriltag_image(const uint8_t* buffer, struct image_u8* apriltag_image) {
    // Copy the buffer contents into the apriltag image buffer
    // TODO: Evaluate the efficiency of this conversion (converting from yuyv -> rgb -> image_u8)
    for (int y = 0; y < apriltag_image->height; y++) {
        for (int x = 0; x < apriltag_image->width; x++) {
            int i = y * apriltag_image->width + x;
            // Assuming the RGB values are interleaved RGBRGBRGB
            uint8_t r = buffer[3*i];
            uint8_t g = buffer[3*i + 1];
            uint8_t b = buffer[3*i + 2];
            // Convert the pixel to grayscale using the luminosity method
            apriltag_image->buf[y * apriltag_image->stride + x] = (uint8_t)(0.21*r + 0.72*g + 0.07*b);
        }
    }

    return 0;
}

int is_valid_id(int id) {
    return id >= 1 && id < 9;
}

int detect_apriltag(struct image_u8* apriltag_image, apriltag_detector_t* td) {
    int detected_tag_id = -1;
    zarray_t* detections = apriltag_detector_detect(td, apriltag_image);
    for (int i = 0; i < zarray_size(detections); ++i) {
        apriltag_detection_t* detection;
        zarray_get(detections, i, &detection);

        // TODO: Can the hamming value be negative?
        if (detection->hamming == 0 || detection->hamming == 1 && is_valid_id(detection->id)) {
            detected_tag_id = detection->id;
            break;
        }
    }

    return detected_tag_id;
}