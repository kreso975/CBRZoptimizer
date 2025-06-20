#include "rar_handle.h"
#include "functions.h"
#include <windows.h>
#include <shlwapi.h> // For PathFindFileNameW if needed
#include <stdio.h>   // For swprintf, etc.
#include <string.h>
#include "unrar.h"

#include "gui.h"

#include <windows.h>
#include "unrar.h" // Ensure this header matches your DLL version

// Typedefs for dynamic linking
typedef HANDLE(PASCAL *RAROPENARCHIVEEX)(struct RAROpenArchiveDataEx *);
typedef int(PASCAL *RARREADHEADEREX)(HANDLE, struct RARHeaderDataEx *);
typedef int(PASCAL *RARPROCESSFILEW)(HANDLE, int, const wchar_t *, const wchar_t *);
typedef int(PASCAL *RARCLOSEARCHIVE)(HANDLE);

BOOL extract_unrar_dll(HWND hwnd, const wchar_t *archive_path, const wchar_t *unused_dest_folder)
{
    // Build base extraction folder like extract_cbr()
    wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH];
    wcscpy(cleanDir, archive_path);
    wchar_t *ext = wcsrchr(cleanDir, L'.');
    if (ext && _wcsicmp(ext, L".cbr") == 0)
        *ext = L'\0';

    swprintf(baseFolder, MAX_PATH, L"%s\\%s", TMP_FOLDER, wcsrchr(cleanDir, L'\\') + 1);

    if (GetFileAttributesW(baseFolder) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(baseFolder, NULL))
        {
            MessageBoxW(hwnd, L"Failed to create extraction directory", baseFolder, MB_OK | MB_ICONERROR);
            return FALSE;
        }
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting (DLL): ", baseFolder);
    }

    HMODULE hUnrar = LoadLibraryW(L"UnRAR64.dll");
    if (!hUnrar)
    {
        DWORD err = GetLastError();
        wchar_t msg[256];
        swprintf(msg, 256, L"❌ LoadLibraryW failed. Error code: %lu", err);
        MessageBoxW(hwnd, msg, L"UnRAR.dll Load Failed", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Load DLL function pointers
    RAROPENARCHIVEEX fnRAROpenArchiveEx = (RAROPENARCHIVEEX)GetProcAddress(hUnrar, "RAROpenArchiveEx");
    RARREADHEADEREX  fnRARReadHeaderEx  = (RARREADHEADEREX) GetProcAddress(hUnrar, "RARReadHeaderEx");
    RARPROCESSFILEW  fnRARProcessFileW  = (RARPROCESSFILEW) GetProcAddress(hUnrar, "RARProcessFileW");
    RARCLOSEARCHIVE  fnRARCloseArchive  = (RARCLOSEARCHIVE) GetProcAddress(hUnrar, "RARCloseArchive");

    if (!fnRAROpenArchiveEx || !fnRARReadHeaderEx || !fnRARProcessFileW || !fnRARCloseArchive)
    {
        MessageBoxW(hwnd, L"⚠️ One or more functions missing from UnRAR.dll.", L"Link Error", MB_OK | MB_ICONERROR);
        FreeLibrary(hUnrar);
        return FALSE;
    }

    struct RAROpenArchiveDataEx openArchive = {0};
    struct RARHeaderDataEx header = {0};

    openArchive.ArcNameW = (wchar_t *)archive_path; // Cast removes const warning
    openArchive.OpenMode = RAR_OM_EXTRACT;

    HANDLE hArchive = fnRAROpenArchiveEx(&openArchive);
    if (!hArchive || openArchive.OpenResult != 0)
    {
        MessageBoxW(hwnd, L"❌ Failed to open archive.", archive_path, MB_OK | MB_ICONERROR);
        FreeLibrary(hUnrar);
        return FALSE;
    }

    while (fnRARReadHeaderEx(hArchive, &header) == 0)
    {
        if (fnRARProcessFileW(hArchive, RAR_EXTRACT, baseFolder, NULL) != 0)
        {
            MessageBoxW(hwnd, L"⚠️ Could not extract file:", header.FileNameW, MB_OK | MB_ICONWARNING);
        }
    }

    fnRARCloseArchive(hArchive);
    FreeLibrary(hUnrar);

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting (DLL): ", L"📂 Flattening image folders...");
    flatten_and_clean_folder(baseFolder, baseFolder);

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting (DLL): ", L"✅ Archive extracted via UnRAR.dll");
    return TRUE;
}



BOOL extract_cbr(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir)
{
   wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH], command[MAX_PATH];
   wcscpy(cleanDir, file_path);

   wchar_t *ext = wcsrchr(cleanDir, L'.');
   if (ext && _wcsicmp(ext, L".cbr") == 0)
      *ext = L'\0';

   swprintf(baseFolder, MAX_PATH, L"%s\\%s", TMP_FOLDER, wcsrchr(cleanDir, L'\\') + 1);

   if (GetFileAttributesW(baseFolder) == INVALID_FILE_ATTRIBUTES)
   {
      if (!CreateDirectoryW(baseFolder, NULL))
      {
         MessageBoxW(hwnd, L"Failed to create extraction directory", baseFolder, MB_OK | MB_ICONERROR);
         return FALSE;
      }
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", baseFolder);
   }

   if (wcslen(WINRAR_PATH) == 0)
   {
      MessageBoxW(hwnd, L"WINRAR_PATH is not set in config.ini", L"Configuration Error", MB_OK | MB_ICONERROR);
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"❌ WINRAR_PATH is not set.");
      return FALSE;
   }

   DWORD attrib = GetFileAttributesW(WINRAR_PATH);
   if (attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY))
   {
      MessageBoxW(hwnd, L"The specified WINRAR_PATH does not exist or is not a file.", L"Invalid Path", MB_OK | MB_ICONERROR);
      return FALSE;
   }

   swprintf(command, MAX_PATH, L"\"%s\" x \"%s\" \"%s\"", WINRAR_PATH, file_path, baseFolder);
   STARTUPINFOW si = {sizeof(si)};
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

   flatten_and_clean_folder(baseFolder, baseFolder); // ✅ Corrected call

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"✅ Extraction complete.");
   wcscpy(final_dir, baseFolder);
   return TRUE;
}