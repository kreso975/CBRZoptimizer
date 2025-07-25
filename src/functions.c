#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <shobjidl.h> // For IFileDialog
#include <shlwapi.h>  // If not already present
#include <commdlg.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>

#include "functions.h"
#include "gui.h"
#include "resource.h"
#include "versioninfo.h"
#include "rar_handle.h"
#include "zip_handle.h"
#include "image_handle.h"
#include "pdf_handle.h"
#include "folder_handle.h"
#include "webp_handle.h"
#include "debug.h"

// List of files/extensions to ignore during extraction:
// If isExtension == TRUE, match by file extension (case-insensitive)
// If isExtension == FALSE, match full filename exactly
// Helps eliminate archive noise like Thumbs.db, .nfo, logs, etc.
const SkipEntry g_skipList[] = {
    {L"Thumbs.db", FALSE},
    {L".DS_Store", FALSE},
    {L"desktop.ini", FALSE},
    {L".nfo", TRUE},
    {L".sfv", TRUE},
    {L".log", TRUE},
    {L".ini", TRUE},
    {L".url", TRUE},
    {L".bak", TRUE},
    // Add more as needed
};

const size_t g_skipListCount = sizeof(g_skipList) / sizeof(g_skipList[0]);

BOOL should_skip_file(const wchar_t *filename)
{
    for (size_t i = 0; i < g_skipListCount; ++i)
    {
        const SkipEntry *entry = &g_skipList[i];
        if (entry->isExtension)
        {
            const wchar_t *ext = wcsrchr(filename, L'.');
            if (ext && _wcsicmp(ext, entry->name) == 0)
                return TRUE;
        }
        else
        {
            if (_wcsicmp(filename, entry->name) == 0)
                return TRUE;
        }
    }
    return FALSE;
}

DWORD WINAPI ProcessingThread(LPVOID lpParam)
{
    HWND hwnd = (HWND)lpParam;
    HWND hListBox = GetDlgItem(hwnd, ID_LISTBOX); // Or pass both in a struct
    StartProcessing(hwnd, hListBox);

    // You can post a message back to the window if needed
    PostMessage(hwnd, WM_USER + 1, 0, 0);
    return 0;
}

ArchiveType detect_archive_type(const wchar_t *file_path)
{
    DWORD attr = GetFileAttributesW(file_path);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
        return ARCHIVE_FOLDER;

    FILE *file = _wfopen(file_path, L"rb");
    if (!file)
        return ARCHIVE_UNKNOWN;

    unsigned char signature[8] = {0};
    size_t read = fread(signature, 1, sizeof(signature), file);
    fclose(file);

    if (read >= 4 && signature[0] == 'P' && signature[1] == 'K')
        return ARCHIVE_CBZ;

    if (read >= 5 && memcmp(signature, "\x52\x61\x72\x21\x1A", 5) == 0)
        return ARCHIVE_CBR;

    if (read == 8 && memcmp(signature, "\x52\x61\x72\x21\x1A\x07\x01\x00", 8) == 0)
        return ARCHIVE_CBR;

    if (read >= 4 && memcmp(signature, "%PDF", 4) == 0)
        return ARCHIVE_PDF;

    return ARCHIVE_UNKNOWN;
}

// Helper to validate if WINRAR_PATH is set and points to .exe
//   mode==1 → extract CBR/RAR (accept winrar.exe OR unrar.exe)
//   mode==2 → unzip only     (accept ONLY winrar.exe)
//   mode==3 → compress to RAR(accept ONLY winrar.exe)
BOOL is_valid_winrar(int mode)
{
    // 1) Path must exist
    if (GetFileAttributesW(g_config.WINRAR_PATH) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    // 2) Isolate filename
    const wchar_t *exe = wcsrchr(g_config.WINRAR_PATH, L'\\');
    exe = exe ? exe + 1 : g_config.WINRAR_PATH;

    // 3) Mode‐dependent check
    switch (mode)
    {
    case 1: // extract
        return _wcsicmp(exe, L"winrar.exe") == 0 || _wcsicmp(exe, L"unrar.exe") == 0;

    case 2: // unzip
    case 3: // compress
        return _wcsicmp(exe, L"winrar.exe") == 0;

    default:
        return FALSE;
    }
}

BOOL is_valid_mutool()
{
    const wchar_t *exe = wcsrchr(g_config.MUTOOL_PATH, L'\\');
    exe = exe ? exe + 1 : g_config.MUTOOL_PATH;

    return wcslen(g_config.MUTOOL_PATH) > 0 &&
           GetFileAttributesW(g_config.MUTOOL_PATH) != INVALID_FILE_ATTRIBUTES &&
           !(GetFileAttributesW(g_config.MUTOOL_PATH) & FILE_ATTRIBUTE_DIRECTORY) &&
           _wcsicmp(exe, L"mutool.exe") == 0;
}

void get_clean_name(wchar_t *path)
{
    wchar_t *ext = wcsrchr(path, L'.');
    if (ext && (_wcsicmp(ext, L".cbr") == 0 ||
                _wcsicmp(ext, L".cbz") == 0 ||
                _wcsicmp(ext, L".rar") == 0 ||
                _wcsicmp(ext, L".zip") == 0 ||
                _wcsicmp(ext, L".pdf") == 0))
    {
        *ext = L'\0'; // Strip known extension
    }
}

BOOL safe_decode_filename(const char *input, wchar_t *output, int fallbackIndex)
{
    char roundtrip[MAX_PATH];
    int valid = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1, output, MAX_PATH);

    if (valid > 0 && !wcschr(output, L'�') && !wcschr(output, L'?'))
    {
        // Round-trip validation: re-encode and compare
        WideCharToMultiByte(CP_UTF8, 0, output, -1, roundtrip, MAX_PATH, NULL, NULL);
        if (strcmp(roundtrip, input) == 0)
            return TRUE;
    }

    // Try fallback decoding using Windows-1250
    valid = MultiByteToWideChar(1250, 0, input, -1, output, MAX_PATH);
    if (valid > 0 && !wcschr(output, L'�') && !wcschr(output, L'?'))
        return TRUE;

    // Fallback: generate a safe dummy name
    swprintf(output, MAX_PATH, L"_badfilename_%03d.bin", fallbackIndex);
    return FALSE;
}

void TrimTrailingWhitespace(wchar_t *str)
{
    size_t len = wcslen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' '))
        str[--len] = '\0';
}

void flatten_and_clean_folder(const wchar_t *source, const wchar_t *target, wchar_t *final_folder_name)
{
    if (!source || !target || !final_folder_name)
        return;

    wchar_t cleanSource[MAX_PATH];
    wcscpy_s(cleanSource, MAX_PATH, source);
    TrimTrailingWhitespace(cleanSource);

    wchar_t cleanTarget[MAX_PATH];
    wcscpy_s(cleanTarget, MAX_PATH, target);
    TrimTrailingWhitespace(cleanTarget);

    // Store the actual folder name used
    wcscpy_s(final_folder_name, MAX_PATH, cleanTarget);

    CreateDirectoryW(cleanTarget, NULL); // Ensure target exists

    if (wcslen(cleanSource) + 3 >= MAX_PATH)
    {
        DEBUG_PRINTF(L"[FLATTEN] ❌ Source path too long: %ls\n", cleanSource);
        return;
    }

    wchar_t search[MAX_PATH];
    swprintf(search, MAX_PATH, L"%s\\*", cleanSource);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search, &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DEBUG_PRINTF(L"[FLATTEN] ❌ FindFirstFile failed for: %ls\n", search);
        return;
    }

    do
    {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
            continue;

        wchar_t fullPath[MAX_PATH];
        swprintf(fullPath, MAX_PATH, L"%s\\%s", cleanSource, ffd.cFileName);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            flatten_and_clean_folder(fullPath, cleanTarget, final_folder_name);
            delete_folder_recursive(fullPath);
        }
        else
        {
            const wchar_t *ext = wcsrchr(ffd.cFileName, L'.');
            if (ext && (_wcsicmp(ext, L".jpg") == 0 ||
                        _wcsicmp(ext, L".jpeg") == 0 ||
                        _wcsicmp(ext, L".png") == 0 ||
                        _wcsicmp(ext, L".bmp") == 0))
            {
                wchar_t dest[MAX_PATH];
                swprintf(dest, MAX_PATH, L"%s\\%s", cleanTarget, ffd.cFileName);
                if (!MoveFileExW(fullPath, dest, MOVEFILE_REPLACE_EXISTING))
                {
#if defined(DEBUG) || defined(_DEBUG)
                    DWORD err = GetLastError();
                    DEBUG_PRINTF(L"[FLATTEN] ⚠️ MoveFileEx failed! Error: %lu\n", err);
#endif
                }
            }
        }

    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);
}

BOOL delete_folder_recursive(const wchar_t *path)
{
    if (!path || wcslen(path) == 0)
        return FALSE;

    // Prepare buffer with double null-termination
    wchar_t temp[MAX_PATH + 2] = {0};
    wcsncpy(temp, path, MAX_PATH);
    size_t len = wcslen(temp);

    if (len > 0 && temp[len - 1] != L'\\')
    {
        temp[len] = L'\\';
        temp[len + 1] = L'\0';
    }

    SHFILEOPSTRUCTW fileOp = {0};
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = temp; // double null-terminated
    fileOp.fFlags = FOF_NO_UI | FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

    int result = SHFileOperationW(&fileOp);

    // Optional: Handle errors (result == 0 means success)
    if (result != 0)
    {
        return FALSE;
        // You can log or fallback here if deletion failed
        // MessageBoxW(NULL, L"Failed to delete folder.", L"Error", MB_OK | MB_ICONERROR);
    }
    return TRUE;
}

void process_file(HWND hwnd, const wchar_t *file_path)
{
    wchar_t base[MAX_PATH];

    // ✅ Extract just the filename from the full path
    const wchar_t *file_name = wcsrchr(file_path, L'\\');
    file_name = file_name ? file_name + 1 : file_path;

    wcscpy(base, file_name);
    get_clean_name(base); // ✅ Strips extension from filename only
    TrimTrailingWhitespace(base);

    wchar_t extracted_dir[MAX_PATH], archive_name[MAX_PATH];

    // g_config.TMP_FOLDER if not set we must have some DIR
    swprintf(extracted_dir, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, base);
    swprintf(archive_name, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, base);

    TrimTrailingWhitespace(extracted_dir); // Fix potential space at end
    TrimTrailingWhitespace(archive_name);

    if (g_StopProcessing)
        return;

    // Sometimes file extension is not set correctly, so we need to detect it
    ArchiveType type = detect_archive_type(file_path);
    BOOL extracted = FALSE;

    if (type == ARCHIVE_CBR)
    {
        // Case #1: CBR/RAR extraction
        extracted = is_valid_winrar(1) ? extract_cbr(hwnd, file_path, extracted_dir) : extract_unrar_dll(hwnd, file_path, extracted_dir);
    }
    else if (type == ARCHIVE_CBZ)
    {
        // Case #2: ZIP extraction / we need winrar.exe to do it
        BOOL hasWinRAR = is_valid_winrar(2);

        BOOL has7zip = wcslen(g_config.SEVEN_ZIP_PATH) > 0 &&
                       GetFileAttributesW(g_config.SEVEN_ZIP_PATH) != INVALID_FILE_ATTRIBUTES &&
                       !(GetFileAttributesW(g_config.SEVEN_ZIP_PATH) & FILE_ATTRIBUTE_DIRECTORY);

        if (hasWinRAR)
            extracted = extract_external_cbz(hwnd, file_path, extracted_dir, EXTERNAL_APP_WINRAR);
        else if (!extracted && has7zip)
            extracted = extract_external_cbz(hwnd, file_path, extracted_dir, EXTERNAL_APP_7ZIP);
        else
            extracted = extract_cbz(hwnd, file_path, extracted_dir); // fallback
    }
    else if (type == ARCHIVE_PDF)
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"Detected PDF file.");

        extracted = pdf_extract_images(hwnd, file_path, extracted_dir);

        if (!extracted)
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ Failed to extract images.");
        else
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"✅ Images extracted.");
    }
    else if (type == ARCHIVE_FOLDER)
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"\xD83D\xDCC1 Folder: ", L"Detected folder.");

        extracted = copy_folder_to_tmp(file_path, extracted_dir);

        if (!extracted)
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"\xD83D\xDCC1 Folder: ", L"❌ Failed to copy folder.");
        else
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"\xD83D\xDCC1 Folder: ", L"✅ Folder copied. Ready for processing.");
    }

    if (!extracted)
    {
        DEBUG_PRINT(L"Extracting Failed!");
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"Failed!");
        return;
    }

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"Unpacking complete.");

    if (g_StopProcessing)
        return;

    // Cover image extraction
    if (g_config.extractCover)
        preserve_only_cover_image(extracted_dir); // 🧹 Keep only the first image

    if (g_StopProcessing)
        return;

    DEBUG_PRINT(g_config.runImageOptimizer ? L"[DEBUG] RunImageOptimizer = TRUE\n" : L"[DEBUG] RunImageOptimizer = FALSE\n");

    // Image optimization
    if (g_config.runImageOptimizer)
    {
        if (wcslen(g_config.IMAGEMAGICK_PATH) == 0 ||
            GetFileAttributesW(g_config.IMAGEMAGICK_PATH) == INVALID_FILE_ATTRIBUTES ||
            (GetFileAttributesW(g_config.IMAGEMAGICK_PATH) & FILE_ATTRIBUTE_DIRECTORY))
        {
            DEBUG_PRINT(L"ImageOptimizer3: ON\n");
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Falling back to STB optimizer...");
            if (!fallback_optimize_images(hwnd, extracted_dir))
                return;
        }
        else
        {
            DEBUG_PRINT(L"ImageOptimizer4: ON\n");
            if (!optimize_images(hwnd, extracted_dir))
                return;
        }

        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Image optimization completed.");
    }

    if (g_StopProcessing)
        return;

    // Preserve only the cover image if enabled
    if (g_config.extractCover)
    {
        if (!extract_cover_image(extracted_dir, g_config.OUTPUT_FOLDER))
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Cover extraction: ", L"⚠️ Failed to copy cover image.");
        else
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Cover extraction: ", L"✅ Cover image copied.");
    }

    if (g_StopProcessing)
        return;

    // Output format
    if (g_config.runCompressor && !g_config.extractCover)
    {
        wchar_t selectedText[32] = L"";
        int selected = (int)SendMessageW(hOutputType, CB_GETCURSEL, 0, 0);
        SendMessageW(hOutputType, CB_GETLBTEXT, (WPARAM)(INT_PTR)selected, (LPARAM)selectedText);

        BOOL useCBR = FALSE;
        BOOL usePDF = (_wcsicmp(selectedText, L"PDF") == 0);

        if ((_wcsicmp(selectedText, L"CBR") == 0) ||
            (_wcsicmp(selectedText, L"Keep original") == 0 && type == ARCHIVE_CBR))
        {
            useCBR = is_valid_winrar(3);
        }

        if (usePDF)
        {
            if (!is_valid_mutool())
            {
                SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ MuPDF not available.");
                return;
            }

            if (!pdf_create_from_images(hwnd, extracted_dir, archive_name))
                return;

            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"✅ PDF created.");
        }
        else if (useCBR)
        {
            if (!create_cbr_archive(hwnd, extracted_dir, archive_name))
                return;
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WinRAR: ", L"Completed");
        }
        else
        {
            if (!create_cbz_archive(hwnd, extracted_dir, archive_name))
                return;
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"7-Zip: ", L"Completed");
        }
    }

    if (g_config.extractCover)
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Compressor: ", L"⏭️ Skipped due to cover extraction mode.");

    // Cleanup if selected
    if (!g_config.keepExtracted)
    {
        // 🧪 Confirm we're entering the block
        //MessageBox(hwnd, L"🧹 Cleanup triggered: keepExtracted is FALSE", L"Debug", MB_OK | MB_ICONINFORMATION);

        DWORD attr = GetFileAttributesW(extracted_dir);
        if (attr == INVALID_FILE_ATTRIBUTES)
        {
            wchar_t msg[MAX_PATH + 64];
            swprintf(msg, MAX_PATH + 64, L"❌ Folder does not exist:\n%s", extracted_dir);
            //MessageBox(hwnd, msg, L"Delete Folder", MB_OK | MB_ICONERROR);
        }
        else if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            wchar_t msg[MAX_PATH + 64];
            swprintf(msg, MAX_PATH + 64, L"⚠️ Path is not a directory:\n%s", extracted_dir);
            //MessageBox(hwnd, msg, L"Delete Folder", MB_OK | MB_ICONWARNING);
        }
        else
        {
            // 🧪 Attempt deletion
            BOOL deleted = delete_folder_recursive(extracted_dir);
            if (!deleted)
            {
                wchar_t msg[MAX_PATH + 64];
                swprintf(msg, MAX_PATH + 64, L"❌ Failed to delete folder:\n%s", extracted_dir);
                //MessageBox(hwnd, msg, L"Delete Folder", MB_OK | MB_ICONERROR);
            }
            else
            {
                wchar_t msg[MAX_PATH + 64];
                swprintf(msg, MAX_PATH + 64, L"✅ Folder deleted:\n%s", extracted_dir);
                //MessageBox(hwnd, msg, L"Delete Folder", MB_OK | MB_ICONINFORMATION);
            }
        }
    }
}

// Start Processing
void StartProcessing(HWND hwnd, HWND hListBox)
{
    LRESULT lTotal = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    int total = (int)lTotal;

    int processed = 0;

    while (!g_StopProcessing && SendMessage(hListBox, LB_GETCOUNT, 0, 0) > 0)
    {
        wchar_t file_path[MAX_PATH];
        SendMessageW(hListBox, LB_GETTEXT, 0, (LPARAM)file_path);

        if (g_StopProcessing)
        {
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"CANCELLED", L"");
            MessageBeep(MB_ICONEXCLAMATION); // play warning sound
            MessageBoxCentered(hwnd, L"Processing was canceled.", L"Info", MB_OK | MB_ICONEXCLAMATION);
            return;
        }

        wchar_t progress[64];
        swprintf(progress, sizeof(progress), L"%d/%d", processed + 1, total);
        SendStatus(hwnd, WM_UPDATE_PROCESSING_TEXT, L"", progress);

        process_file(hwnd, file_path);
        SendMessage(hListBox, LB_DELETESTRING, 0, 0);

        processed++;
    }

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"DONE", L"");
    MessageBeep(MB_ICONINFORMATION); // play warning sound
    MessageBoxCentered(hwnd, L"Processing Complete!", L"Info", MB_OK | MB_ICONINFORMATION);
    PostMessage(hwnd, WM_PROCESSING_DONE, 0, 0);
}

void BrowseFolder(HWND hwnd, wchar_t *targetPath)
{
    HRESULT hr;
    IFileDialog *pfd = NULL;

    // Initialize COM
    hr = CoInitialize(NULL);
    if (FAILED(hr))
        return;

    // Create the FileOpenDialog object
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileDialog, (void **)&pfd);
    if (SUCCEEDED(hr))
    {
        // Set the options to pick folders
        DWORD options = 0;
        hr = pfd->lpVtbl->GetOptions(pfd, &options);
        if (SUCCEEDED(hr))
        {
            pfd->lpVtbl->SetOptions(pfd, options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }

        // Show the dialog
        hr = pfd->lpVtbl->Show(pfd, hwnd);
        if (SUCCEEDED(hr))
        {
            IShellItem *psi = NULL;
            hr = pfd->lpVtbl->GetResult(pfd, &psi);
            if (SUCCEEDED(hr))
            {
                PWSTR folderPath = NULL;
                hr = psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH, &folderPath);
                if (SUCCEEDED(hr))
                {
                    wcsncpy(targetPath, folderPath, MAX_PATH);
                    CoTaskMemFree(folderPath);
                }
                psi->lpVtbl->Release(psi);
            }
        }
        pfd->lpVtbl->Release(pfd);
    }

    // Uninitialize COM
    CoUninitialize();
}

void BrowseFile(HWND hwnd, wchar_t *targetPath)
{
    HRESULT hr;
    IFileDialog *pfd = NULL;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
        return;

    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileDialog, (void **)&pfd);
    if (SUCCEEDED(hr))
    {
        // Set file type filters
        COMDLG_FILTERSPEC filters[] = {
            {L"Executables", L"*.exe"},
            {L"All Files", L"*.*"}};
        pfd->lpVtbl->SetFileTypes(pfd, ARRAYSIZE(filters), filters);
        pfd->lpVtbl->SetTitle(pfd, L"Select an executable");

        // Show the dialog
        hr = pfd->lpVtbl->Show(pfd, hwnd);
        if (SUCCEEDED(hr))
        {
            IShellItem *psi = NULL;
            hr = pfd->lpVtbl->GetResult(pfd, &psi);
            if (SUCCEEDED(hr))
            {
                PWSTR filePath = NULL;
                hr = psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH, &filePath);
                if (SUCCEEDED(hr))
                {
                    wcsncpy(targetPath, filePath, MAX_PATH);
                    CoTaskMemFree(filePath);
                }
                psi->lpVtbl->Release(psi);
            }
        }
        pfd->lpVtbl->Release(pfd);
    }

    CoUninitialize();
}

void OpenFileDialog(HWND hwnd, HWND hListBox)
{
    OPENFILENAMEW ofn;
    wchar_t fileNames[MAX_PATH * 50] = {0}; // Large buffer to hold multiple file paths
    wchar_t filter[256] = {0};

    // Build dynamic filter
    wcscpy(filter, L"Comic & Archive Files (*.cbr;*.cbz;*.rar;*.zip");
    if (wcslen(g_config.MUTOOL_PATH) > 0 && GetFileAttributesW(g_config.MUTOOL_PATH) != INVALID_FILE_ATTRIBUTES &&
        !(GetFileAttributesW(g_config.MUTOOL_PATH) & FILE_ATTRIBUTE_DIRECTORY))
    {
        wcscat(filter, L";*.pdf");
    }
    wcscat(filter, L")\0*.cbr;*.cbz;*.rar;*.zip");
    if (wcslen(g_config.MUTOOL_PATH) > 0 && GetFileAttributesW(g_config.MUTOOL_PATH) != INVALID_FILE_ATTRIBUTES &&
        !(GetFileAttributesW(g_config.MUTOOL_PATH) & FILE_ATTRIBUTE_DIRECTORY))
    {
        wcscat(filter, L";*.pdf");
    }
    wcscat(filter, L"\0All Files\0*.*\0");

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileNames;
    ofn.nMaxFile = sizeof(fileNames);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileNameW(&ofn))
    {
        wchar_t *p = fileNames + wcslen(fileNames) + 1;

        if (*p == L'\0')
        {
            // Single file selected
            AddUniqueToListBox(hwnd, hListBox, fileNames);
        }
        else
        {
            // Multiple files selected
            wchar_t folder[MAX_PATH];
            wcscpy(folder, fileNames);

            while (*p)
            {
                wchar_t fullPath[MAX_PATH];
                swprintf(fullPath, MAX_PATH, L"%s\\%s", folder, p);
                AddUniqueToListBox(hwnd, hListBox, fullPath);
                p += wcslen(p) + 1;
            }
        }
    }
}

int CompareVersions(const char *v1, const char *v2)
{
    int major1, minor1, patch1;
    int major2, minor2, patch2;

    sscanf(v1, "%d.%d.%d", &major1, &minor1, &patch1);
    sscanf(v2, "%d.%d.%d", &major2, &minor2, &patch2);

    if (major1 != major2)
        return major1 - major2;
    if (minor1 != minor2)
        return minor1 - minor2;
    return patch1 - patch2;
}

/**
 * @brief Checks for a newer version of the application by querying the latest release from GitHub.
 *
 * This function retrieves the current application version, fetches the latest release information
 * from the GitHub API, and compares the versions. If a newer version is available, it notifies
 * the user via a centered message box. If the application is up-to-date and the 'silent' parameter
 * is FALSE, it informs the user that they are running the latest version.
 *
 * @param hwnd   Handle to the parent window for message boxes.
 * @param silent If TRUE, suppresses notifications when the application is up-to-date. Used on startup
 */
void CheckForUpdate(HWND hwnd, BOOL silent)
{
    // Get current version from executable
    AppVersionInfo info;
    GetAppVersionFields(&info);

    char currentVersion[64];
    wcstombs(currentVersion, info.FileVersion, sizeof(currentVersion));
    currentVersion[sizeof(currentVersion) - 1] = '\0';

    // Strip leading 'v' if present
    if (currentVersion[0] == 'v' || currentVersion[0] == 'V')
    {
        memmove(currentVersion, currentVersion + 1, strlen(currentVersion));
    }

    HINTERNET hInternet = InternetOpenW(L"CBRZoptimizer Updater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet)
    {
        HINTERNET hConnect = InternetOpenUrlW(hInternet, L"https://api.github.com/repos/kreso975/CBRZoptimizer/releases/latest",
            NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);

        if (hConnect)
        {
            char buffer[4096];
            DWORD bytesRead;
            if (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead))
            {
                buffer[bytesRead] = '\0';

                const char *tagStart = strstr(buffer, "\"tag_name\":\"");
                if (tagStart)
                {
                    tagStart += strlen("\"tag_name\":\"");
                    const char *tagEnd = strchr(tagStart, '"');
                    if (tagEnd)
                    {
                        char latestVersion[32] = {0};
                        strncpy(latestVersion, tagStart, (size_t)(tagEnd - tagStart));
                        latestVersion[tagEnd - tagStart] = '\0';

                        // Strip leading 'v' if present
                        char strippedVersion[32];
                        if (latestVersion[0] == 'v' || latestVersion[0] == 'V')
                        {
                            strncpy(strippedVersion, latestVersion + 1, sizeof(strippedVersion) - 1);
                        }
                        else
                        {
                            strncpy(strippedVersion, latestVersion, sizeof(strippedVersion) - 1);
                        }
                        strippedVersion[sizeof(strippedVersion) - 1] = '\0';

                        // Compare versions
                        if (CompareVersions(currentVersion, strippedVersion) < 0)
                        {
                            wchar_t wCurrent[64], wLatest[64];
                            mbstowcs(wCurrent, currentVersion, sizeof(wCurrent) / sizeof(wchar_t));
                            mbstowcs(wLatest, strippedVersion, sizeof(wLatest) / sizeof(wchar_t));

                            wchar_t message[256];
                            swprintf(message, sizeof(message) / sizeof(wchar_t),
                                     L"A new version is available: %s\nYou are running: %s",
                                     wLatest, wCurrent);

                            MessageBoxCentered(hwnd, message, L"Update Available", MB_OK | MB_ICONINFORMATION);
                        }
                        else if (!silent)
                        {
                            MessageBoxCentered(hwnd, L"You are running the latest version.", L"No Update", MB_OK | MB_ICONINFORMATION);
                        }
                    }
                }
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
}
