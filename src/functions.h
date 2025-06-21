#pragma once

#include <windows.h>


typedef struct {
    const wchar_t  *labelText;
    const wchar_t  *configKey;
    int y;
    HWND *hCheckbox;
    HWND *hLabel;
} LabelCheckboxPair;

extern LabelCheckboxPair controls[];
extern const int controlCount;

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
void load_config_values(HWND hTmpFolder, HWND hOutputFolder, HWND hWinrarPath, HWND hSevenZipPath, HWND hImageMagickPath, 
                        HWND hImageDpi, HWND hImageSize, HWND hImageQualityValue, HWND hImageQualitySlider,
                        HWND hOutputRunImageOptimizer, HWND hOutputRunCompressor, HWND hOutputKeepExtracted, HWND hOutputType,
                        int controlCount);


DWORD WINAPI ProcessingThread(LPVOID lpParam);

