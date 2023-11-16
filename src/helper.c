#include "helper.h"

int write_grayscale_image_to_file(const char* filename, struct image_u8* grayscale_img) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Unable to open file for writing");
        return -1;
    }

    int maximum_color_value = 255;
    fprintf(file, "P5\n%d %d\n%d\n", grayscale_img->width, grayscale_img->height, maximum_color_value);

    size_t bytes_written = 0;
    for (int i = 0; i < grayscale_img->height; ++i) {
        bytes_written += fwrite(grayscale_img->buf + i * grayscale_img->stride, 1, grayscale_img->width, file);
    }

    if (bytes_written != grayscale_img->width * grayscale_img->height) {
        perror("Error writing grayscale data to PPM file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}