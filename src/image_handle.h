#ifndef IMAGE_HANDLE_H
#define IMAGE_HANDLE_H

#include <windows.h>

// Define this struct if it's not declared elsewhere:
typedef struct {
    wchar_t image_path[MAX_PATH];
    HWND hwnd;
    int target_width;
    int target_height;
    BOOL keep_aspect;
    BOOL allow_upscale;
} ImageTask;

extern void TrimTrailingWhitespace(wchar_t *str);

// Expose the thread entry point
BOOL is_image_file(const wchar_t *filename);
BOOL IsWebPImage(const BYTE *buffer, DWORD size);
BOOL preserve_only_cover_image(const wchar_t *folderPath);
BOOL extract_cover_image(const wchar_t *folderPath, const wchar_t *coverFolderPath);
DWORD WINAPI OptimizeImageThread(LPVOID lpParam);
void stb_write_func(void *context, void *data, int size);
BOOL fallback_optimize_images(HWND hwnd, const wchar_t *folder);
BOOL optimize_images(HWND hwnd, const wchar_t *image_folder);

#endif // IMAGE_HANDLE_H
