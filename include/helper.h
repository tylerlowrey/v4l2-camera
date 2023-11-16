#pragma once
#include <stdint.h>
#include <stdio.h>
#include "apriltag_detection.h"

int write_grayscale_image_to_file(const char* filename, struct image_u8 *grayscale_img);
