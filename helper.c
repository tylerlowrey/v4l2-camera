#include "helper.h"

uint8_t clamp_rgb(double value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

void yuyv_to_rgb(uint8_t y1, uint8_t u, uint8_t y2, uint8_t v, uint8_t* rgb) {
    // Conversion coefficients from YUV to RGB
    const double YtoR = 1.164383;
    const double UtoG = -0.391762;
    const double UtoB = 2.017232;
    const double VtoR = 1.596027;
    const double VtoG = -0.812968;

    // Calculate Cb and Cr from U and V (with 128 subtracted)
    double cb = u - 128;
    double cr = v - 128;

    // Convert YUYV to RGB for the first pixel
    rgb[0] = clamp_rgb(YtoR * (y1 - 16) + VtoR * cr);
    rgb[1] = clamp_rgb(YtoR * (y1 - 16) + UtoG * cb + VtoG * cr);
    rgb[2] = clamp_rgb(YtoR * (y1 - 16) + UtoB * cb);

    // Convert YUYV to RGB for the second pixel
    rgb[3] = clamp_rgb(YtoR * (y2 - 16) + VtoR * cr);
    rgb[4] = clamp_rgb(YtoR * (y2 - 16) + UtoG * cb + VtoG * cr);
    rgb[5] = clamp_rgb(YtoR * (y2 - 16) + UtoB * cb);
}