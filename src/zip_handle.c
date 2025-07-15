#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h> // For PathFindFileNameW if needed
#include <stdio.h>   // For swprintf, etc.
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

#include "miniz.h"

#pragma GCC diagnostic pop
#include "functions.h"
#include "gui.h"
#include "zip_handle.h"
#include "debug.h"

BOOL extract_cbz(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir)
{
    wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH], status_msg[256];
    wcscpy(cleanDir, file_path);

    // 🧼 Remove known archive extension
    get_clean_name(cleanDir);

    const wchar_t *rawFolder = wcsrchr(cleanDir, L'\\');
    const wchar_t *folderStart = rawFolder ? rawFolder + 1 : cleanDir;

    wchar_t trimmedFolder[MAX_PATH];
    wcsncpy(trimmedFolder, folderStart, MAX_PATH);
    trimmedFolder[MAX_PATH - 1] = L'\0';

    wchar_t *tail = trimmedFolder + wcslen(trimmedFolder) - 1;
    while (tail > trimmedFolder && iswspace(*tail))
        *tail-- = L'\0';

    swprintf(baseFolder, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, trimmedFolder);

    if (GetFileAttributesW(baseFolder) == INVALID_FILE_ATTRIBUTES && !CreateDirectoryW(baseFolder, NULL))
    {
        MessageBoxCentered(hwnd, L"Failed to create extraction directory", baseFolder, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    char zip_utf8[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, file_path, -1, zip_utf8, MAX_PATH, NULL, NULL);

    mz_zip_archive zip = {0};
    if (!mz_zip_reader_init_file(&zip, zip_utf8, 0))
    {
        MessageBoxCentered(hwnd, L"Failed to open CBZ archive", file_path, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    int filesExtracted = 0;
    mz_uint fileCount = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < fileCount; ++i)
    {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat))
            continue;
        if (mz_zip_reader_is_file_a_directory(&zip, i))
            continue;

        char relpath_utf8[MAX_PATH];
        mz_zip_reader_get_filename(&zip, i, relpath_utf8, MAX_PATH);

        char *slash = strrchr(relpath_utf8, '/');
        if (!slash)
            slash = strrchr(relpath_utf8, '\\');
        char folder_utf8[MAX_PATH] = "", file_utf8[MAX_PATH] = "";
        if (slash)
        {
            size_t len = (size_t)(slash - relpath_utf8);
            strncpy(folder_utf8, relpath_utf8, len);
            folder_utf8[len] = '\0';
            strcpy(file_utf8, slash + 1);
        }
        else
        {
            strcpy(file_utf8, relpath_utf8);
        }

        wchar_t decoded_folder[MAX_PATH] = L"", decoded_file[MAX_PATH] = L"";
        safe_decode_filename(folder_utf8, decoded_folder, (int)i);
        safe_decode_filename(file_utf8, decoded_file, (int)i);

        if (should_skip_file(decoded_file))
        {
            DEBUG_PRINTF(L"⏭️ Skipping unwanted file: %s\n", decoded_file);
            continue;
        }

        swprintf(status_msg, 256, L"[%u/%u] %s\\%s", i + 1, fileCount, decoded_folder, decoded_file);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", status_msg);

        wchar_t fullDestW[MAX_PATH];
        if (wcslen(decoded_folder))
            swprintf(fullDestW, MAX_PATH, L"%s\\%s\\%s", baseFolder, decoded_folder, decoded_file);
        else
            swprintf(fullDestW, MAX_PATH, L"%s\\%s", baseFolder, decoded_file);

        wchar_t tempPath[MAX_PATH];
        wcscpy(tempPath, fullDestW);
        wchar_t *p = wcsrchr(tempPath, L'\\');
        if (p)
        {
            *p = L'\0';
            wchar_t *s = tempPath + wcslen(baseFolder) + 1;
            while (s && *s)
            {
                wchar_t *subslash = wcschr(s, L'\\');
                if (subslash)
                    *subslash = L'\0';
                CreateDirectoryW(tempPath, NULL);
                if (subslash)
                    *subslash = L'\\';
                s = subslash ? subslash + 1 : NULL;
            }
        }

        char fullDest_utf8[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, fullDestW, -1, fullDest_utf8, MAX_PATH, NULL, NULL);

        if (mz_zip_reader_extract_to_file(&zip, i, fullDest_utf8, 0))
        {
            ++filesExtracted;
        }
        else
        {
            swprintf(status_msg, 256, L"❌ Failed to extract: %s", fullDestW);
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", status_msg);
        }
    }

    mz_zip_reader_end(&zip);

    if (filesExtracted == 0)
    {
        DEBUG_PRINT(L"❌ CBZ archive contained no extractable files.");
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"❌ No valid files extracted from CBZ.");
        return FALSE;
    }

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"📂 Flattening image folders...");
    wchar_t cleanPath[MAX_PATH];
    flatten_and_clean_folder(baseFolder, baseFolder, cleanPath);
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, baseFolder, NULL);

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"✅ CBZ extraction complete.");
    wcscpy(final_dir, baseFolder);
    return TRUE;
}

BOOL extract_external_cbz(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir, ExternalApp externalApp)
{
    wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH], command[MAX_PATH];
    wcscpy(cleanDir, file_path);

    // 🧼 Remove known archive extension
    get_clean_name(cleanDir);

    swprintf(baseFolder, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, wcsrchr(cleanDir, L'\\') + 1);

    if (GetFileAttributesW(baseFolder) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(baseFolder, NULL))
        {
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting fail: ", baseFolder);
            MessageBeep(MB_ICONERROR);
            MessageBoxCentered(hwnd, L"Failed to create extraction directory", L"Extract Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", baseFolder);
    }

    const wchar_t *extractor = NULL;

    if (externalApp == EXTERNAL_APP_WINRAR)
    {
        if (wcslen(g_config.WINRAR_PATH) == 0)
        {
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"❌ WINRAR_PATH is not set.");
            MessageBeep(MB_ICONERROR);
            MessageBoxCentered(hwnd, L"WINRAR_PATH is not set in config.ini", L"Configuration Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        extractor = g_config.WINRAR_PATH;
        swprintf(command, MAX_PATH, L"\"%s\" x \"%s\" \"%s\"", extractor, file_path, baseFolder);
    }
    else if (externalApp == EXTERNAL_APP_7ZIP)
    {
        if (wcslen(g_config.SEVEN_ZIP_PATH) == 0)
        {
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"❌ SEVEN_ZIP_PATH is not set.");
            MessageBeep(MB_ICONERROR);
            MessageBoxCentered(hwnd, L"SEVEN_ZIP_PATH is not set in config.ini", L"Configuration Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        extractor = g_config.SEVEN_ZIP_PATH;
        swprintf(command, MAX_PATH, L"\"%s\" x \"%s\" -o\"%s\" -y", extractor, file_path, baseFolder);
    }

    DWORD attrib = GetFileAttributesW(extractor);
    if (attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        MessageBeep(MB_ICONERROR);
        MessageBoxCentered(hwnd, L"The specified extraction tool does not exist or is not a file.", L"Invalid Path", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;

    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"❌ Failed to launch extraction tool.");
        return FALSE;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"📂 Flattening image folders...");
    wchar_t cleanPath[MAX_PATH];
    flatten_and_clean_folder(baseFolder, baseFolder, cleanPath);

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"✅ Extraction complete.");
    wcscpy(final_dir, baseFolder);
    return TRUE;
}

BOOL create_cbz_with_miniz(HWND hwnd, const wchar_t *folder, const wchar_t *output_cbz)
{
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    wchar_t status_buf[256];

    DEBUG_PRINTF(L"📦 Starting CBZ creation from folder: %s", folder);
    DEBUG_PRINTF(L"📤 Target CBZ path: %s", output_cbz);

    // Convert output CBZ filename to UTF-8 for MiniZ
    char output_cbz_utf8[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, output_cbz, -1, output_cbz_utf8, MAX_PATH, NULL, NULL);

    if (!mz_zip_writer_init_file(&zip, output_cbz_utf8, 0))
    {
        DEBUG_PRINTF(L"❌ Failed to initialize ZIP writer for: %s", output_cbz);
        return FALSE;
    }

    wchar_t search_path[MAX_PATH];
    swprintf(search_path, MAX_PATH, L"%s\\*", folder);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search_path, &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DEBUG_PRINTF(L"❌ Failed to enumerate files in: %s", folder);
        return FALSE;
    }

    do
    {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // 🔒 Skip unwanted files
            if (should_skip_file(ffd.cFileName))
            {
                DEBUG_PRINTF(L"⏭️ Skipping unwanted file: %s", ffd.cFileName);
                continue;
            }

            wchar_t filepathW[MAX_PATH], archiveW[MAX_PATH];
            swprintf(filepathW, MAX_PATH, L"%s\\%s", folder, ffd.cFileName);
            wcscpy(archiveW, ffd.cFileName);

            // Convert both paths to UTF-8 for MiniZ
            char filepath_utf8[MAX_PATH], archive_utf8[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, filepathW, -1, filepath_utf8, MAX_PATH, NULL, NULL);
            WideCharToMultiByte(CP_UTF8, 0, archiveW, -1, archive_utf8, MAX_PATH, NULL, NULL);

            swprintf(status_buf, 256, L"Adding %s", ffd.cFileName);
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", status_buf);
            DEBUG_PRINTF(L"➕ Adding file to archive: %s", filepathW);

            if (!mz_zip_writer_add_file(&zip, archive_utf8, filepath_utf8, NULL, 0, MZ_BEST_COMPRESSION))
            {
                DEBUG_PRINTF(L"❌ Failed to add file to archive: %s", filepathW);
                mz_zip_writer_end(&zip);
                FindClose(hFind);
                return FALSE;
            }
        }
    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);

    if (!mz_zip_writer_finalize_archive(&zip))
    {
        DEBUG_PRINTF(L"❌ Failed to finalize archive: %s", output_cbz);
        mz_zip_writer_end(&zip);
        return FALSE;
    }

    mz_zip_writer_end(&zip);
    DEBUG_PRINTF(L"✅ Archive finalized: %s", output_cbz);
    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", L"Archive finalized successfully.");

    if (g_config.OUTPUT_FOLDER[0] != L'\0')
    {
        const wchar_t *cbz_name = wcsrchr(output_cbz, L'\\');
        cbz_name = cbz_name ? cbz_name + 1 : output_cbz;

        wchar_t dest_cbzW[MAX_PATH];
        swprintf(dest_cbzW, MAX_PATH, L"%s\\%s", g_config.OUTPUT_FOLDER, cbz_name);

        if (MoveFileW(output_cbz, dest_cbzW))
        {
            DEBUG_PRINTF(L"📁 Moved archive to OUTPUT_FOLDER: %s", dest_cbzW);
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", L"✔ Archive moved to OUTPUT_FOLDER.");
        }
        else
        {
            DEBUG_PRINTF(L"⚠️ Failed to move archive to OUTPUT_FOLDER: %s", dest_cbzW);
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", L"⚠️ Failed to move archive to OUTPUT_FOLDER.");
        }
    }
    else
    {
        DEBUG_PRINT(L"📂 OUTPUT_FOLDER not set. Archive remains in TMP.");
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", L"📁 OUTPUT_FOLDER not set. Leaving archive in TMP.");
    }

    return TRUE;
}

// Create CBZ Archive
BOOL create_cbz_archive(HWND hwnd, const wchar_t *image_folder, const wchar_t *archive_name)
{
    wchar_t cleanName[MAX_PATH], zip_file[MAX_PATH], cbz_file[MAX_PATH], command[1024];
    wchar_t buffer[4096];
    DWORD bytesRead;
    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    wcscpy(cleanName, archive_name);
    // 🧼 Remove known archive extension
    get_clean_name(cleanName);

    swprintf(zip_file, MAX_PATH, L"%s.zip", cleanName);
    swprintf(cbz_file, MAX_PATH, L"%s.cbz", cleanName);

    if (wcslen(g_config.SEVEN_ZIP_PATH) > 0)
    {
        DWORD attrib = GetFileAttributesW(g_config.SEVEN_ZIP_PATH);
        if (!(attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY)))
        {
            if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
                goto fallback;

            STARTUPINFOW si = {0};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi;
            si.hStdOutput = hWritePipe;
            si.hStdError = hWritePipe;
            si.wShowWindow = SW_HIDE;
            si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

            swprintf(command, 1024, L"\"%s\" a -mx9 \"%s\" \"%s\"",
                     g_config.SEVEN_ZIP_PATH, zip_file, image_folder);

            if (CreateProcessW(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                               NULL, NULL, &si, &pi))
            {
                CloseHandle(hWritePipe);
                if (ReadFile(hReadPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, NULL))
                {
                    buffer[bytesRead / sizeof(wchar_t)] = L'\0';
                    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"7-Zip: ", buffer);
                }
                else
                {
                    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"7-Zip: ", L"7-Zip might have run successfully.");
                }

                CloseHandle(hReadPipe);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                MoveFileW(zip_file, cbz_file);
                SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"7-Zip: ", L"Renaming from .zip to .cbz");

                if (g_config.OUTPUT_FOLDER[0] != L'\0')
                {
                    const wchar_t *cbz_name = wcsrchr(cbz_file, L'\\');
                    cbz_name = cbz_name ? cbz_name + 1 : cbz_file;

                    wchar_t dest_cbz[MAX_PATH];
                    swprintf(dest_cbz, MAX_PATH, L"%s\\%s", g_config.OUTPUT_FOLDER, cbz_name);
                    MoveFileW(cbz_file, dest_cbz);

                    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"7-Zip: ", L"✔ Archive moved to OUTPUT_FOLDER.");
                }
                else
                {
                    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"7-Zip: ", L"📁 OUTPUT_FOLDER not set. Leaving archive in TMP.");
                }

                return TRUE;
            }
        }
    }

fallback:
    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Fallback: ", L"7-Zip unavailable, using miniz.");
    return create_cbz_with_miniz(hwnd, image_folder, cbz_file);
}