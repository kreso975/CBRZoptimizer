#ifndef WEBP_HANDLE_H
#define WEBP_HANDLE_H

#include <stddef.h>
#include <stdint.h>

unsigned char* webp_decode_from_memory(const BYTE* buffer, DWORD size, int* width, int* height, int* channels);
BOOL EncodeImageToWebPMemory(HWND hwnd, const uint8_t *data, int width, int height, BOOL useRGBA, uint8_t **out_data, size_t *out_size);
DWORD WINAPI ConvertToWebPThread(LPVOID lpParam);
BOOL webp_encode_and_write(HWND hwnd, const wchar_t *inputPath, const uint8_t *rgb_buf, int width, int height);
BOOL convert_images_to_webp(HWND hwnd, const wchar_t* folder_path);

#endif // WEBP_HANDLE_H