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
void SendStatus(HWND hwnd, UINT messageId, const wchar_t *prefix, const wchar_t *info);
BOOL find_folder_with_images(const wchar_t *basePath, wchar_t *outPath, int depth);
void flatten_and_clean_folder(const wchar_t *source, const wchar_t *target);
void delete_folder_recursive(const wchar_t *path);
void replace_all(wchar_t *str, const wchar_t *old_sub, const wchar_t *new_sub);
BOOL extract_cbz(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir);
BOOL extract_cbr(HWND hwnd,const wchar_t *file_path, wchar_t *extracted_dir);
BOOL fallback_optimize_images(HWND hwnd, const wchar_t *folder);
BOOL optimize_images(HWND hwnd,const wchar_t *image_folder);
BOOL create_cbz_with_miniz(HWND hwnd, const wchar_t *folder, const wchar_t *output_cbz);
BOOL create_cbz_archive(HWND hwnd, const wchar_t *image_folder, const wchar_t *archive_name);
void process_file(HWND hwnd,const wchar_t *file_path);
void ProcessDroppedFiles(HWND hwnd, HWND hListBox, HDROP hDrop);
void StartProcessing(HWND hwnd, HWND hListBox);
void BrowseFolder(HWND hwnd, wchar_t *targetPath);
void BrowseFile(HWND hwnd, wchar_t *targetPath);
void OpenFileDialog(HWND hwnd, HWND hListBox);
void RemoveSelectedItems(HWND hListBox);
HBITMAP LoadBMP(const wchar_t *filename);
void update_output_type_dropdown(HWND hOutputType, const wchar_t *winrarPath);
void load_config_values(HWND hTmpFolder, HWND hOutputFolder, HWND hWinrarPath,
                        HWND hSevenZipPath, HWND hImageMagickPath, HWND hImageDpi,
                        HWND hImageSize, HWND hImageQualityValue, HWND hImageQualitySlider,
                        HWND hOutputRunExtract, HWND hOutputRunImageOptimizer,
                        HWND hOutputRunCompressor, HWND hOutputKeepExtracted, HWND hOutputType,
                        int controlCount);


DWORD WINAPI ProcessingThread(LPVOID lpParam);

