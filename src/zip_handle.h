#pragma once

#include <windows.h>

// Status callback (optional - safe to remove if not used elsewhere)
extern void SendStatus(HWND hwnd, UINT msg, const wchar_t *prefix, const wchar_t *message);
extern void flatten_and_clean_folder(const wchar_t *source, const wchar_t *target);

BOOL extract_cbz(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir);
BOOL create_cbz_with_miniz(HWND hwnd, const wchar_t *folder, const wchar_t *output_cbz);
BOOL create_cbz_archive(HWND hwnd, const wchar_t *image_folder, const wchar_t *archive_name);