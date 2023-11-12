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
int NUM_BUFFERS = 20;

void process_frame(struct buffer* buffer, struct image_u8* apriltag_image);
int write_frame_to_file(const char* filename, uint8_t* rgb_data);
int detect_april_tag(struct image_u8* image, apriltag_detector_t* detector);

int main() {
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

    for (int i = 0; i < 10000; ++i) {
        int buffer_index = dequeue_buffer(camera_fd);
        while (buffer_index == -1) {
            printf("Received -1 response from de-queuing buffer\n");
            perror("");
            usleep(100000);
            buffer_index = dequeue_buffer(camera_fd);
        }

        printf("Dequeued buffer with index: %d\n", buffer_index);

        process_frame(&buffers[buffer_index], grayscale_image_buffers[buffer_index]);
        int detected_apriltag_id = detect_april_tag(
        grayscale_image_buffers[buffer_index],
        apriltag_detector
        );

        if (detected_apriltag_id == -1) {
            printf("No april tag detected\n");
        } else {
            printf("Detected april tag with ID: %d\n", detected_apriltag_id);
        }

        while (requeue_buffer(camera_fd, buffer_index) == -1) {
            printf("WARN: Unable to requeue buffer with index %d\n", buffer_index);
            usleep(100000);
        }

        printf("re-queued buffer with index %d\n", buffer_index);

        usleep(100000);
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

// TODO: Write tests for this function using unity
void process_frame(struct buffer* buffer, struct image_u8* apriltag_image) {
    uint32_t apriltag_image_size = apriltag_image->stride * apriltag_image->height;
    int32_t apriltag_image_padding_size = apriltag_image->stride - apriltag_image->width;
    uint8_t *yuyv_buffer = (uint8_t*)buffer->start;
    size_t yuyv_buffer_length = buffer->length;

    if (yuyv_buffer_length != FRAME_WIDTH * FRAME_HEIGHT * 2 || apriltag_image_size * 2 != yuyv_buffer_length) {
        printf("Could not process frame because of improper buffer lengths\n"
               "Actual YUYV Buffer Length: %zu\nExpected YUYV Buffer Length: %d\nAprilTag Image Buffer Length: %d\n",
               yuyv_buffer_length, FRAME_WIDTH * FRAME_HEIGHT * 2, apriltag_image_size);
        return;
    }

    for (size_t i = 0, j = 0; i < yuyv_buffer_length; i += 4, j += 2) {
        apriltag_image->buf[j] = yuyv_buffer[i];
        apriltag_image->buf[j+1] = yuyv_buffer[i + 2];

        if (((j + 2) % apriltag_image->stride) == 0) {
            memset(apriltag_image->buf, 0, apriltag_image_padding_size);
            j += apriltag_image_padding_size;
        }
    }
}

int write_frame_to_file(const char* filename, uint8_t* rgb_data) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Unable to open file for writing");
        return -1;
    }

    int maximum_color_value = 255;
    fprintf(file, "P6\n%d %d\n%d\n", FRAME_WIDTH, FRAME_HEIGHT, maximum_color_value);

    size_t bytes_written = fwrite(rgb_data, 1, FRAME_WIDTH * FRAME_HEIGHT * 3, file);
    if (bytes_written != FRAME_WIDTH * FRAME_HEIGHT * 3) {
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

int detect_april_tag(struct image_u8* image, apriltag_detector_t* detector) {
    int detected_tag_id = -1;
    zarray_t* detections = apriltag_detector_detect(detector, image);
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