#ifndef WEBP_HANDLE_H
#define WEBP_HANDLE_H

#include <stddef.h>
#include <stdint.h>

// Encode RGBA image to WebP format
// Returns pointer to encoded data (must be freed with WebPFree)
uint8_t* webp_encode_image(const uint8_t* rgba, int width, int height, int stride, int quality, size_t* output_size);

// Decode WebP image to RGBA
// Returns pointer to RGBA data (must be freed with WebPFree)
uint8_t* webp_decode_image(const uint8_t* webp_data, size_t webp_size, int* width, int* height);


BOOL webp_encode_and_write(HWND hwnd, const wchar_t *inputPath, const uint8_t *rgb_buf, int width, int height);

BOOL convert_images_to_webp(HWND hwnd, const wchar_t* folder_path);

#endif // WEBP_HANDLE_H