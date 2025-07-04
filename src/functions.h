#pragma once

#include <windows.h>

typedef enum {
    IMAGE_TYPE_PORTRAIT = 0,
    IMAGE_TYPE_LANDSCAPE = 1,
    IMAGE_TYPE_COUNT
} ImageType;

typedef struct {
    const wchar_t *label;
    ImageType type;
} ImageTypeEntry;

extern const ImageTypeEntry g_ImageTypeOptions[];

typedef enum
{
   ARCHIVE_UNKNOWN,
   ARCHIVE_CBR,
   ARCHIVE_CBZ
} ArchiveType;

typedef struct {
    const wchar_t *name;
    BOOL isExtension;
} SkipEntry;

extern const SkipEntry g_skipList[];
extern const size_t g_skipListCount;

BOOL should_skip_file(const wchar_t *filename);

BOOL is_valid_winrar(int mode);
void get_clean_name(const wchar_t *file_path, wchar_t *base);
BOOL safe_decode_filename(const char *input, wchar_t *output, int fallbackIndex);

void flatten_and_clean_folder(const wchar_t *source, const wchar_t *target, wchar_t *final_folder_name);
void delete_folder_recursive(const wchar_t *path);
void process_file(HWND hwnd, const wchar_t *file_path);
void StartProcessing(HWND hwnd, HWND hListBox);
void BrowseFolder(HWND hwnd, wchar_t *targetPath);
void BrowseFile(HWND hwnd, wchar_t *targetPath);
void OpenFileDialog(HWND hwnd, HWND hListBox);
void TrimTrailingWhitespace(wchar_t *str);

DWORD WINAPI ProcessingThread(LPVOID lpParam);
ArchiveType detect_archive_type(const wchar_t *file_path);

