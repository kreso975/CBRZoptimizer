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
   // Build base extraction folder like extract_cbr()
   wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH];
   wcscpy(cleanDir, archive_path);
   wchar_t *ext = wcsrchr(cleanDir, L'.');
   if (ext && _wcsicmp(ext, L".cbr") == 0)
      *ext = L'\0';

   swprintf(baseFolder, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, wcsrchr(cleanDir, L'\\') + 1);

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
      MessageBeep(MB_ICONERROR); // Play error sound
      MessageBoxCentered(hwnd, L"UnRAR.dll Load Failed", L"UnRAR.dll", MB_OK | MB_ICONERROR);
      return FALSE;
   }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
   // Load DLL function pointers
   RAROPENARCHIVEEX fnRAROpenArchiveEx = (RAROPENARCHIVEEX)GetProcAddress(hUnrar, "RAROpenArchiveEx");
   RARREADHEADEREX fnRARReadHeaderEx = (RARREADHEADEREX)GetProcAddress(hUnrar, "RARReadHeaderEx");
   RARPROCESSFILEW fnRARProcessFileW = (RARPROCESSFILEW)GetProcAddress(hUnrar, "RARProcessFileW");
   RARCLOSEARCHIVE fnRARCloseArchive = (RARCLOSEARCHIVE)GetProcAddress(hUnrar, "RARCloseArchive");
#pragma GCC diagnostic pop

   if (!fnRAROpenArchiveEx || !fnRARReadHeaderEx || !fnRARProcessFileW || !fnRARCloseArchive)
   {
      // MessageBoxW(hwnd, L"⚠️ One or more functions missing from UnRAR.dll.", L"Link Error", MB_OK | MB_ICONERROR);
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
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"❌ Failed to open archive.", archive_path);
      MessageBeep(MB_ICONERROR); // Play error sound
      MessageBoxCentered(hwnd, L"Failed to open archive.", L"Archive Error", MB_OK | MB_ICONERROR);
      FreeLibrary(hUnrar);
      return FALSE;
   }

   while (fnRARReadHeaderEx(hArchive, &header) == 0)
   {
      // Show file being extracted
      wchar_t statusMsg[MAX_PATH + 64];
      swprintf(statusMsg, MAX_PATH + 64, L"🔸 Extracting: %s", header.FileNameW);
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting (DLL): ", statusMsg);

      if (fnRARProcessFileW(hArchive, RAR_EXTRACT, baseFolder, NULL) != 0)
      {
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"⚠️ Could not extract file:", header.FileNameW);
         MessageBeep(MB_ICONERROR); // Play error sound
         MessageBoxCentered(hwnd, L"Could not extract file", L"Extract Error", MB_OK | MB_ICONERROR);
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

   swprintf(baseFolder, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, wcsrchr(cleanDir, L'\\') + 1);

   if (GetFileAttributesW(baseFolder) == INVALID_FILE_ATTRIBUTES)
   {
      if (!CreateDirectoryW(baseFolder, NULL))
      {
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting fail: ", baseFolder);
         MessageBeep(MB_ICONERROR); // Play error sound
         MessageBoxCentered(hwnd, L"Failed to create extraction directory", L"Extract Error", MB_OK | MB_ICONERROR);
         return FALSE;
      }
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", baseFolder);
   }

   if (wcslen(g_config.WINRAR_PATH) == 0)
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"❌ WINRAR_PATH is not set.");
      MessageBeep(MB_ICONERROR); // Play error sound
      MessageBoxCentered(hwnd, L"WINRAR_PATH is not set in config.ini", L"Configuration Error", MB_OK | MB_ICONERROR);
      return FALSE;
   }

   DWORD attrib = GetFileAttributesW(g_config.WINRAR_PATH);
   if (attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY))
   {
      MessageBeep(MB_ICONERROR); // Play error sound
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

   flatten_and_clean_folder(baseFolder, baseFolder); // ✅ Corrected call

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"✅ Extraction complete.");
   wcscpy(final_dir, baseFolder);
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
