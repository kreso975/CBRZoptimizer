#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h> // For PathFindFileNameW if needed
#include <stdio.h>   // For swprintf, etc.
#include <stdlib.h>
#include <string.h>

#include "functions.h"
#include "gui.h"
#include "webp_handle.h"
#include "debug.h"
#include <webp/encode.h>
#include <webp/decode.h>
#include <webp/types.h>  // Optional, if you use WebP-specific types

uint8_t* webp_encode_image(const uint8_t* rgba, int width, int height, int stride, int quality, size_t* output_size) {
    if (!rgba || width <= 0 || height <= 0 || !output_size) return NULL;

    uint8_t* output_data = NULL;
    float quality_factor = (float)quality;

    size_t size = WebPEncodeRGBA(rgba, width, height, stride, quality_factor, &output_data);
    if (size == 0 || output_data == NULL) return NULL;

    *output_size = size;
    return output_data;
}


uint8_t* webp_decode_image(const uint8_t* webp_data, size_t webp_size, int* width, int* height) {
    if (!webp_data || webp_size == 0 || !width || !height) {
        return NULL;
    }

    return WebPDecodeRGBA(webp_data, webp_size, width, height);
}