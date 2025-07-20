#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h> // For SHChangeNotify
#include <wchar.h>

#include "pdf_handle.h"
#include "functions.h"
#include "gui.h"
#include "debug.h"

BOOL pdf_extract_images(HWND hwnd, const wchar_t *pdf_path, const wchar_t *output_folder)
{
    DEBUG_PRINTF(L"[PDF] Starting extraction: %s → %s\n", pdf_path, output_folder);

    // Validate mutool path from config
    if (wcslen(g_config.MUTOOL_PATH) == 0)
    {
        DEBUG_PRINT(L"[PDF] MUTOOL_PATH is not set.\n");
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ MUTOOL_PATH is not set.");
        MessageBeep(MB_ICONERROR);
        MessageBoxCentered(hwnd, L"MUTOOL_PATH is not set in config.ini", L"Configuration Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    DWORD attrib = GetFileAttributesW(g_config.MUTOOL_PATH);
    if (attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        DEBUG_PRINTF(L"[PDF] Invalid mutool path: %s\n", g_config.MUTOOL_PATH);
        MessageBeep(MB_ICONERROR);
        MessageBoxCentered(hwnd, L"The specified MUTOOL_PATH does not exist or is not a file.", L"Invalid Path", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Create output folder if needed
    if (GetFileAttributesW(output_folder) == INVALID_FILE_ATTRIBUTES)
    {
        DEBUG_PRINTF(L"[PDF] Creating output folder: %s\n", output_folder);
        if (!CreateDirectoryW(output_folder, NULL))
        {
            DEBUG_PRINT(L"[PDF] Failed to create output folder.\n");
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ Failed to create output folder.");
            MessageBeep(MB_ICONERROR);
            MessageBoxCentered(hwnd, L"Failed to create output folder for PDF images.", L"Folder Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    // Build output file pattern
    wchar_t output_pattern[MAX_PATH];
    swprintf(output_pattern, MAX_PATH, L"%s\\page_%%03d.png", output_folder);

    // Build command line
    wchar_t command[1024];
    swprintf(command, 1024, L"\"%s\" draw -o \"%s\" -r 150 \"%s\"",
             g_config.MUTOOL_PATH, output_pattern, pdf_path);

    DEBUG_PRINTF(L"[PDF] Command: %s\n", command);

    // Launch mutool process
    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {0};

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"\xD83D\xDCC4 Rendering pages...");

    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        DEBUG_PRINT(L"[PDF] Failed to launch mutool process.\n");
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ Failed to launch mutool.");
        MessageBeep(MB_ICONERROR);
        MessageBoxCentered(hwnd, L"Failed to launch mutool.exe", L"Execution Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    DEBUG_PRINT(L"[PDF] mutool process completed.\n");

    // Check if output folder has images
    wchar_t test_image[MAX_PATH];
    swprintf(test_image, MAX_PATH, L"%s\\page_001.png", output_folder);

    if (GetFileAttributesW(test_image) == INVALID_FILE_ATTRIBUTES)
    {
        DEBUG_PRINT(L"[PDF] No images found after rendering.\n");
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ No images found after rendering.");
        MessageBeep(MB_ICONERROR);
        MessageBoxCentered(hwnd, L"No images were generated from the PDF.", L"Render Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    DEBUG_PRINT(L"[PDF] Images extracted successfully.\n");
    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"✅ Images extracted successfully.");
    return TRUE;
}

BOOL pdf_create_from_images(HWND hwnd, const wchar_t *image_folder, const wchar_t *archive_name)
{
    wchar_t cleanName[MAX_PATH], pdf_file[MAX_PATH], command[8192];
    wcscpy(cleanName, archive_name);
    get_clean_name(cleanName); // Strip known extensions

    swprintf(pdf_file, MAX_PATH, L"%s.pdf", cleanName);

    if (!is_valid_mutool())
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ MUTOOL_PATH is invalid or not set.");
        return FALSE;
    }

    // Scan folder and collect supported image files
    WIN32_FIND_DATAW fd;
    wchar_t searchPath[MAX_PATH];
    swprintf(searchPath, MAX_PATH, L"%s\\*", image_folder);

    HANDLE hFind = FindFirstFileW(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ No images found.");
        return FALSE;
    }

    // Build file list
    wchar_t fileList[8192] = L"";
    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        const wchar_t *ext = PathFindExtensionW(fd.cFileName);
        if (_wcsicmp(ext, L".jpg") == 0 || _wcsicmp(ext, L".jpeg") == 0 ||
            _wcsicmp(ext, L".png") == 0 || _wcsicmp(ext, L".bmp") == 0 ||
            _wcsicmp(ext, L".tif") == 0 || _wcsicmp(ext, L".tiff") == 0)
        {
            wchar_t fullPath[MAX_PATH];
            swprintf(fullPath, MAX_PATH, L"\"%s\\%s\" ", image_folder, fd.cFileName);
            wcscat(fileList, fullPath);
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (wcslen(fileList) == 0)
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ No supported images found.");
        return FALSE;
    }

    // Build mutool command
    swprintf(command, 8192, L"\"%s\" convert -o \"%s\" %s", g_config.MUTOOL_PATH, pdf_file, fileList);
    // create -o output.pdf (Get-ChildItem *.jpg | Sort-Object Name | ForEach-Object { $_.Name })"
    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"📄 Creating PDF from images...");

    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"❌ Failed to start MuPDF.");
        return FALSE;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"✅ PDF created.");

    if (g_config.OUTPUT_FOLDER[0] != L'\0')
    {
        const wchar_t *pdf_name = wcsrchr(pdf_file, L'\\');
        pdf_name = pdf_name ? pdf_name + 1 : pdf_file;

        wchar_t dest_pdf[MAX_PATH];
        swprintf(dest_pdf, MAX_PATH, L"%s\\%s", g_config.OUTPUT_FOLDER, pdf_name);
        MoveFileW(pdf_file, dest_pdf);

        // 🔄 Refresh Explorer for the destination folder
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, g_config.OUTPUT_FOLDER, NULL);

        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"✔ PDF moved to OUTPUT_FOLDER.");
    }
    else
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"PDF: ", L"📁 OUTPUT_FOLDER not set. Leaving PDF in TMP.");
    }

    return TRUE;
}