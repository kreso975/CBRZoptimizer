#pragma once

#include <windows.h>

BOOL pdf_extract_images(HWND hwnd, const wchar_t *pdf_path, const wchar_t *output_folder);
BOOL pdf_create_from_images(HWND hwnd, const wchar_t *image_folder, const wchar_t *archive_name);