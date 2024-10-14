#include "classifier/ei_classifier_types.h"
#include "edge-impulse-sdk/dsp/image/processing.hpp"
#include "source/bitmap_helpers.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include <cstdint>

// raw frame buffer from the camera
#define FRAME_BUFFER_COLS 320
#define FRAME_BUFFER_ROWS 240
uint8_t image_rgb888_packed[400 * 400 * 3] = { 0 }; // oversize buffer for space for fit longest

// Can also resize into a new buffer
// uint8_t resized_image[400 * 400 * 3] = {0}; // buffer for resized image

using namespace ei::image::processing;

int test_resize(int_least16_t mode, const char *outputFileName, int desiredWidth, int desiredHeight)
{
    // fill frame buffer with some example data... This is normally done by the camera.
    for (size_t row = 0; row < FRAME_BUFFER_ROWS; row++) {
        for (size_t col = 0; col < FRAME_BUFFER_COLS; col++) {
            // change color a bit (light -> dark from top->down, so we know if the image looks good quickly)
            uint8_t blue_intensity =
                (uint8_t)((255.0f / (float)(FRAME_BUFFER_ROWS)) * (float)(row));
            uint8_t green_intensity =
                (uint8_t)((255.0f / (float)(FRAME_BUFFER_COLS)) * (float)(col));

            constexpr int R_OFFSET = 2, G_OFFSET = 1, B_OFFSET = 0;
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col) * 3 + R_OFFSET] =
                0; //red is zero for test
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col) * 3 + G_OFFSET] = green_intensity;
            image_rgb888_packed[((row * FRAME_BUFFER_COLS) + col) * 3 + B_OFFSET] = blue_intensity;
        }
    }

    int b =
        create_bitmap_file("input.bmp", image_rgb888_packed, FRAME_BUFFER_COLS, FRAME_BUFFER_ROWS);
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

    // for (int i = 0; i < desiredHeight; i++) {
    //     for (int j = 0; j < desiredWidth; j++) {
    //         int index = (i * desiredWidth + j) * 3;
    //         printf("%d, %d, %d,", (int)image_rgb888_packed[index], (int)image_rgb888_packed[index + 1], (int)image_rgb888_packed[index + 2]);
    //     }
    //     printf("\n");
    // }

    return res;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    // we already have a RGB888 buffer, so recalculate offset into pixel index
    size_t pixel_ix = offset * 3;
    size_t out_ptr_ix = 0;

    while (length != 0)
    {
        // pixels are written into the mantissa of the float
        out_ptr[out_ptr_ix] = (image_rgb888_packed[pixel_ix] << 16) + (image_rgb888_packed[pixel_ix + 1] << 8) + image_rgb888_packed[pixel_ix + 2];

        // go to the next pixel
        out_ptr_ix++;
        pixel_ix += 3;
        length--;
    }

    // and done!
    return 0;
}

void demo() {
    // This will crop to maintain aspect ratio
    // Ie, no distortion
    test_resize(EI_CLASSIFIER_RESIZE_FIT_SHORTEST, "shortest.bmp", 200, 200);

    // The following also avoid distortion, but by padding
    // AKA letterboxing
    // Letterboxes on top and bottom, like a movie
    test_resize(EI_CLASSIFIER_RESIZE_FIT_LONGEST, "longest_fat.bmp", 200, 200);
    // Letterboxes on sides, AKA pillarboxing
    test_resize(EI_CLASSIFIER_RESIZE_FIT_LONGEST, "longest_skinny.bmp", 400, 200);

    // No cropping or padding.
    // Will distort the ratio of objects (length v width)
    test_resize(EI_CLASSIFIER_RESIZE_SQUASH, "squash.bmp", 200, 200);
}

int main()
{
    // This demo will write out a file for each mode
    demo();

    // ****** How to use the resize_image function with run_classifier and studio settings
    // Note that the resize_image function can resize the image in place (ie the input and output buffers can be the same)
    ei::image::processing::resize_image_using_mode(
        image_rgb888_packed,
        FRAME_BUFFER_COLS,
        FRAME_BUFFER_ROWS,
        image_rgb888_packed,
        EI_CLASSIFIER_INPUT_WIDTH,
        EI_CLASSIFIER_INPUT_HEIGHT,
        3, // input to run_classifier is always RGB888
        EI_CLASSIFIER_RESIZE_MODE);

    signal_t signal
    {
        // total length in the case of an image classifier is number of pixels, not bytes
        .total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT,
        .get_data = &ei_camera_get_data
    };

    ei_impulse_result_t result = { 0 };
    run_classifier(&signal, &result);
}