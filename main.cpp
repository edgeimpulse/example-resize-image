#include "source/bitmap_helpers.h"
#include "edge-impulse-sdk/dsp/image/processing.hpp"

// raw frame buffer from the camera
#define FRAME_BUFFER_COLS 320
#define FRAME_BUFFER_ROWS 240
uint8_t image_rgb888_packed[400 * 400 * 3] = {0}; // oversize buffer for space for fit longest

// Can also resize into a new buffer
// uint8_t resized_image[400 * 400 * 3] = {0}; // buffer for resized image

using namespace ei::image::processing;

int test_resize(fit_mode_t mode, const char* outputFileName, int desiredWidth, int desiredHeight)
{
    // fill frame buffer with some example data... This is normally done by the camera.
    for (size_t row = 0; row < FRAME_BUFFER_ROWS; row++)
    {
        for (size_t col = 0; col < FRAME_BUFFER_COLS; col++)
        {
            // change color a bit (light -> dark from top->down, so we know if the image looks good quickly)
            uint8_t blue_intensity = (uint8_t)((255.0f / (float)(FRAME_BUFFER_ROWS)) * (float)(row));
            uint8_t green_intensity = (uint8_t)((255.0f / (float)(FRAME_BUFFER_COLS)) * (float)(col));

            constexpr int R_OFFSET = 2, G_OFFSET = 1, B_OFFSET = 0;
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col)*3 + R_OFFSET] = 0; //red is zero for test
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col)*3 + G_OFFSET] = green_intensity;
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col)*3 + B_OFFSET] = blue_intensity;
        }
    }

    int b = create_bitmap_file("input.bmp", image_rgb888_packed, FRAME_BUFFER_COLS, FRAME_BUFFER_ROWS);
    printf("created input.bmp, result code: %d\n", b);

    int pixelSize = 3; // pixel size in bytes, 3 for RGB

    int res = resize_image_using_mode(
        image_rgb888_packed,
        FRAME_BUFFER_COLS,
        FRAME_BUFFER_ROWS,
        image_rgb888_packed,
        desiredWidth,
        desiredHeight,
        pixelSize,
        mode);

    if (res != 0) {
        printf("Error in resize_image_using_mode: %d\n", res);
        return res;
    }

    b = create_bitmap_file(outputFileName, image_rgb888_packed, desiredWidth, desiredHeight);
    printf("created %s, result code: %d\n", outputFileName, b);

    return res;
}

int main() {
    // This will crop to maintain aspect ratio
    // Ie, no distortion
    test_resize(FIT_SHORTEST, "shortest.bmp", 200,200);

    // The following also avoid distortion, but by padding
    // AKA letterboxing
    // Letterboxes on top and bottom, like a movie
    test_resize(FIT_LONGEST, "longest_fat.bmp", 200,200);
    // Letterboxes on sides, AKA pillarboxing
    test_resize(FIT_LONGEST, "longest_skinny.bmp", 400,200);

    // No cropping or padding.
    // Will distort the ratio of objects (length v width)
    test_resize(SQUASH, "squash.bmp",200,200);
}