#include <stdint.h>

uint8_t clamp_rgb(double value);
void yuyv_to_rgb(uint8_t y1, uint8_t u, uint8_t y2, uint8_t v, uint8_t* rgb);
