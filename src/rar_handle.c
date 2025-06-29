#include <windows.h>
#include <shlwapi.h> // For PathFindFileNameW if needed
#include <stdio.h>   // For swprintf, etc.
#include <string.h>

#include "rar_handle.h"
#include "functions.h"
#include "gui.h"
#include "unrar.h" // Ensure this header matches your DLL version
#include "debug.h"

// Typedefs for dynamic linking

typedef HANDLE(PASCAL *RAROPENARCHIVEEX)(struct RAROpenArchiveDataEx *);
typedef int(PASCAL *RARREADHEADEREX)(HANDLE, struct RARHeaderDataEx *);
typedef int(PASCAL *RARPROCESSFILEW)(HANDLE, int, const wchar_t *, const wchar_t *);
typedef int(PASCAL *RARCLOSEARCHIVE)(HANDLE);

BOOL extract_unrar_dll(HWND hwnd, const wchar_t *archive_path, const wchar_t *unused_dest_folder)
{
    (void)unused_dest_folder;

    wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH];
    wcscpy(cleanDir, archive_path);

    // 🧽 Trim trailing whitespace from whole path
    wchar_t *end = cleanDir + wcslen(cleanDir) - 1;
    while (end > cleanDir && iswspace(*end))
        *end-- = L'\0';

    // Remove extension if it's .cbr or .rar
    wchar_t *ext = wcsrchr(cleanDir, L'.');
    if (ext && (_wcsicmp(ext, L".cbr") == 0 || _wcsicmp(ext, L".rar") == 0))
        *ext = L'\0';

    // Extract base name and trim trailing spaces from that as well
    const wchar_t *rawFolder = wcsrchr(cleanDir, L'\\');
    const wchar_t *folderStart = rawFolder ? rawFolder + 1 : cleanDir;

    wchar_t trimmedFolder[MAX_PATH];
    wcsncpy(trimmedFolder, folderStart, MAX_PATH);
    trimmedFolder[MAX_PATH - 1] = L'\0';  // ensure null-termination

    // ✂️ Trim trailing whitespace again from folder name
    wchar_t *folderEnd = trimmedFolder + wcslen(trimmedFolder) - 1;
    while (folderEnd > trimmedFolder && iswspace(*folderEnd))
        *folderEnd-- = L'\0';

    // Build final baseFolder path
    swprintf(baseFolder, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, trimmedFolder);

    if (GetFileAttributesW(baseFolder) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(baseFolder, NULL))
        {
            MessageBoxW(hwnd, L"Failed to create extraction directory", baseFolder, MB_OK | MB_ICONERROR);
            return FALSE;
        }
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting (DLL): ", baseFolder);
    }

    HMODULE hUnrar = LoadLibraryW(UNRAR_DLL_PATH);
    if (!hUnrar)
    {
        DWORD err = GetLastError();
        wchar_t msg[256];
        swprintf(msg, 256, L"❌ LoadLibraryW failed. Error code: %lu", err);
        MessageBeep(MB_ICONERROR);
        MessageBoxCentered(hwnd, L"UnRAR.dll Load Failed", L"UnRAR.dll", MB_OK | MB_ICONERROR);
        return FALSE;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    RAROPENARCHIVEEX fnRAROpenArchiveEx = (RAROPENARCHIVEEX)GetProcAddress(hUnrar, "RAROpenArchiveEx");
    RARREADHEADEREX  fnRARReadHeaderEx  = (RARREADHEADEREX)GetProcAddress(hUnrar, "RARReadHeaderEx");
    RARPROCESSFILEW  fnRARProcessFileW  = (RARPROCESSFILEW)GetProcAddress(hUnrar, "RARProcessFileW");
    RARCLOSEARCHIVE  fnRARCloseArchive  = (RARCLOSEARCHIVE)GetProcAddress(hUnrar, "RARCloseArchive");
#pragma GCC diagnostic pop

    if (!fnRAROpenArchiveEx || !fnRARReadHeaderEx || !fnRARProcessFileW || !fnRARCloseArchive)
    {
        FreeLibrary(hUnrar);
        return FALSE;
    }

    struct RAROpenArchiveDataEx openArchive = {0};
    struct RARHeaderDataEx header = {0};
    openArchive.ArcNameW = (wchar_t *)archive_path;
    openArchive.OpenMode = RAR_OM_EXTRACT;

    HANDLE hArchive = fnRAROpenArchiveEx(&openArchive);
    if (!hArchive || openArchive.OpenResult != 0)
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"❌ Failed to open archive.", archive_path);
        MessageBeep(MB_ICONERROR);
        MessageBoxCentered(hwnd, L"Failed to open archive.", L"Archive Error", MB_OK | MB_ICONERROR);
        FreeLibrary(hUnrar);
        return FALSE;
    }

    while (fnRARReadHeaderEx(hArchive, &header) == 0)
    {
        const wchar_t *originalName = wcsrchr(header.FileNameW, L'\\');
        const wchar_t *filenameOnly = originalName ? originalName + 1 : header.FileNameW;

        wchar_t flatOut[MAX_PATH];
        swprintf(flatOut, MAX_PATH, L"%s\\%s", baseFolder, filenameOnly);

#if defined(DEBUG) || defined(_DEBUG)
        DEBUG_PRINTF(L"[DLL Extract] → Attempting to extract to: %s\n", flatOut);
#endif

        wchar_t statusMsg[MAX_PATH + 64];
        swprintf(statusMsg, MAX_PATH + 64, L"🪪 Target extract path: %s", flatOut);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting (DLL): ", statusMsg);

        if (fnRARProcessFileW(hArchive, RAR_EXTRACT, NULL, flatOut) != 0)
        {
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"⚠️ Could not extract file:", filenameOnly);
            MessageBeep(MB_ICONERROR);
            MessageBoxCentered(hwnd, L"Could not extract file", filenameOnly, MB_OK | MB_ICONERROR);
        }
    }

    fnRARCloseArchive(hArchive);
    FreeLibrary(hUnrar);

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting (DLL): ", L"📂 Flattening image folders...");
    wchar_t cleanPath[MAX_PATH];
    flatten_and_clean_folder(baseFolder, baseFolder, cleanPath);

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting (DLL): ", L"✅ Archive extracted via UnRAR.dll");
    return TRUE;
}

BOOL extract_cbr(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir)
{
   wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH], command[MAX_PATH];
   wcscpy(cleanDir, file_path);

   wchar_t *ext = wcsrchr(cleanDir, L'.');
   if (ext && (_wcsicmp(ext, L".cbr") == 0 || _wcsicmp(ext, L".rar") == 0))
      *ext = L'\0';

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

   if (wcslen(g_config.WINRAR_PATH) == 0)
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"❌ WINRAR_PATH is not set.");
      MessageBeep(MB_ICONERROR);
      MessageBoxCentered(hwnd, L"WINRAR_PATH is not set in config.ini", L"Configuration Error", MB_OK | MB_ICONERROR);
      return FALSE;
   }

   DWORD attrib = GetFileAttributesW(g_config.WINRAR_PATH);
   if (attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY))
   {
      MessageBeep(MB_ICONERROR);
      MessageBoxCentered(hwnd, L"The specified WINRAR_PATH does not exist or is not a file.", L"Invalid Path", MB_OK | MB_ICONERROR);
      return FALSE;
   }

   swprintf(command, MAX_PATH, L"\"%s\" x \"%s\" \"%s\"", g_config.WINRAR_PATH, file_path, baseFolder);
   STARTUPINFOW si = {0};
   si.cb = sizeof(si);
   si.dwFlags = STARTF_USESHOWWINDOW;
   si.wShowWindow = SW_HIDE;
   PROCESS_INFORMATION pi;

   if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"❌ Failed to launch WinRAR.");
      return FALSE;
   }

   WaitForSingleObject(pi.hProcess, INFINITE);
   CloseHandle(pi.hProcess);
   CloseHandle(pi.hThread);

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"📂 Flattening image folders...");

   wchar_t cleaned_path[MAX_PATH] = {0};
   flatten_and_clean_folder(baseFolder, baseFolder, cleaned_path); // Updated to 3-argument version

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"✅ Extraction complete.");

   wcscpy(final_dir, cleaned_path); // Return the actual cleaned path
   return TRUE;
}

BOOL create_cbr_archive(HWND hwnd, const wchar_t *image_folder, const wchar_t *archive_name)
{
   wchar_t cleanName[MAX_PATH], rar_file[MAX_PATH], cbr_file[MAX_PATH], command[1024];
   wcscpy(cleanName, archive_name);
   wchar_t *ext = wcsrchr(cleanName, L'.');
   if (ext && (_wcsicmp(ext, L".cbz") == 0 || _wcsicmp(ext, L".zip") == 0))
      *ext = L'\0';

   swprintf(rar_file, MAX_PATH, L"%s.rar", cleanName);
   swprintf(cbr_file, MAX_PATH, L"%s.cbr", cleanName);

   if (wcslen(g_config.WINRAR_PATH) == 0 ||
       GetFileAttributesW(g_config.WINRAR_PATH) == INVALID_FILE_ATTRIBUTES ||
       (GetFileAttributesW(g_config.WINRAR_PATH) & FILE_ATTRIBUTE_DIRECTORY))
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WinRAR: ", L"❌ WINRAR_PATH is invalid or not set.");
      return FALSE;
   }

   swprintf(command, 1024, L"\"%s\" a -m5 -ep1 -r \"%s\" \"%s\\*\"", g_config.WINRAR_PATH, rar_file, image_folder);

   STARTUPINFOW si = {0};
   si.cb = sizeof(si);
   si.dwFlags = STARTF_USESHOWWINDOW;
   si.wShowWindow = SW_HIDE;
   PROCESS_INFORMATION pi;

   if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WinRAR: ", L"❌ Failed to start WinRAR.");
      return FALSE;
   }

   WaitForSingleObject(pi.hProcess, INFINITE);
   CloseHandle(pi.hProcess);
   CloseHandle(pi.hThread);

   MoveFileW(rar_file, cbr_file);
   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WinRAR: ", L"Renamed .rar to .cbr");

   if (g_config.OUTPUT_FOLDER[0] != L'\0')
   {
      const wchar_t *cbr_name = wcsrchr(cbr_file, L'\\');
      cbr_name = cbr_name ? cbr_name + 1 : cbr_file;

      wchar_t dest_cbr[MAX_PATH];
      swprintf(dest_cbr, MAX_PATH, L"%s\\%s", g_config.OUTPUT_FOLDER, cbr_name);
      MoveFileW(cbr_file, dest_cbr);

      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WinRAR: ", L"✔ Archive moved to OUTPUT_FOLDER.");
   }
   else
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WinRAR: ", L"📁 OUTPUT_FOLDER not set. Leaving archive in TMP.");
   }

   return TRUE;
}
