#pragma once
#include "apriltag/apriltag.h"
#include "apriltag/tag16h5.h"
#include "apriltag/common/image_types.h"
#include "camera.h"

int detect_april_tag(struct image_u8* image, apriltag_detector_t* detector);
void prepare_frame_for_processing(struct buffer* buffer, struct image_u8* apriltag_image);
