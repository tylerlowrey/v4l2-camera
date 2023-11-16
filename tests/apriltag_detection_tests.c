#include "unity.h"
#include "apriltag/common/image_types.h"
#include "apriltag_detection.h"

void setUp() {

}

void tearDown() {

}

void test_process_frame_for_apriltags() {
    struct image_u8* image = image_u8_create(60, 2);
    struct buffer* buffer = (struct buffer*)  malloc(sizeof(struct buffer));
    buffer->length = 60 * 2 * 2;
    buffer->start = malloc(sizeof(uint8_t) * buffer->length);
    uint8_t* yuyv_buffer = buffer->start;
    for (int i = 0; i < buffer->length; i += 4) {
        yuyv_buffer[i] = 0x11;
        yuyv_buffer[i+1] = 0xBB;
        yuyv_buffer[i+2] = 0x22;
        yuyv_buffer[i+3] = 0xAA;
    }
    process_frame_for_apriltags(buffer, image);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_process_frame_for_apriltags);
    return UNITY_END();
}