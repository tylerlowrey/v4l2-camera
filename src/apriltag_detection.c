#include "apriltag_detection.h"

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

void prepare_frame_for_processing(struct buffer* buffer, struct image_u8* apriltag_image) {
    uint32_t apriltag_image_size = apriltag_image->stride * apriltag_image->height;
    int32_t apriltag_image_padding_size = apriltag_image->stride - apriltag_image->width;
    uint8_t* yuyv_buffer = (uint8_t*)buffer->start;
    size_t yuyv_buffer_length = buffer->length;

    if (yuyv_buffer_length != apriltag_image->width * apriltag_image->height * 2) {
        printf("Could not process frame because of improper buffer lengths\n"
               "Actual YUYV Buffer Length: %zu\nExpected YUYV Buffer Length: %d\nAprilTag Image Buffer Length: %d\n",
               yuyv_buffer_length, apriltag_image->width * apriltag_image->height * 2, apriltag_image_size);
        return;
    }

    for (size_t i = 0, j = 0; i < apriltag_image->height; ++i) {
        size_t k = 0 + (apriltag_image->stride * i);
        for (; k < (apriltag_image->width + apriltag_image->stride * i) ; j += 4, k += 2) {
            apriltag_image->buf[k] = yuyv_buffer[j];
            apriltag_image->buf[k+1] = yuyv_buffer[j+2];
        }
        memset(&apriltag_image->buf[k+2], 0, apriltag_image_padding_size);
    }
}