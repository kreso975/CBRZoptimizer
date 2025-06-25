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

BOOL is_zip_archive(const wchar_t *file_path);
BOOL is_valid_winrar();
void get_clean_name(const wchar_t *file_path, wchar_t *base);
void SendStatus(HWND hwnd, UINT messageId, const wchar_t *prefix, const wchar_t *info);
BOOL find_folder_with_images(const wchar_t *basePath, wchar_t *outPath, int depth);
void flatten_and_clean_folder(const wchar_t *source, const wchar_t *target);
void delete_folder_recursive(const wchar_t *path);

void process_file(HWND hwnd, HWND hOutputType, const wchar_t *file_path);
void StartProcessing(HWND hwnd, HWND hOutputType, HWND hListBox);
void BrowseFolder(HWND hwnd, wchar_t *targetPath);
void BrowseFile(HWND hwnd, wchar_t *targetPath);
void OpenFileDialog(HWND hwnd, HWND hListBox);
void AddUniqueToListBox(HWND hwndOwner, HWND hListBox, LPCWSTR itemText);
void ProcessDroppedFiles(HWND hwnd, HWND hListBox, HDROP hDrop);
void RemoveSelectedItems(HWND hListBox);
HBITMAP LoadBMP(const wchar_t *filename);
void update_output_type_dropdown(HWND hOutputType, const wchar_t *winrarPath);



DWORD WINAPI ProcessingThread(LPVOID lpParam);
ArchiveType detect_archive_type(const wchar_t *file_path);

