#include <windows.h>
#include <shlwapi.h> // For PathFindFileNameW if needed
#include <stdio.h>   // For swprintf, etc.
#include <string.h>

#include "miniz.h"
#include "functions.h"
#include "gui.h"
#include "zip_handle.h"

BOOL extract_cbz(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir)
{
   wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH];
   wcscpy(cleanDir, file_path);

   wchar_t *ext = wcsrchr(cleanDir, L'.');
   if (ext && (_wcsicmp(ext, L".cbz") == 0 || _wcsicmp(ext, L".zip") == 0))
      *ext = L'\0';

   swprintf(baseFolder, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, wcsrchr(cleanDir, L'\\') + 1);

   if (GetFileAttributesW(baseFolder) == INVALID_FILE_ATTRIBUTES)
   {
      if (!CreateDirectoryW(baseFolder, NULL))
      {
         MessageBoxW(hwnd, L"Failed to create extraction directory", baseFolder, MB_OK | MB_ICONERROR);
         return FALSE;
      }
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", baseFolder);
   }

   char zip_utf8[MAX_PATH];
   WideCharToMultiByte(CP_UTF8, 0, file_path, -1, zip_utf8, MAX_PATH, NULL, NULL);

   mz_zip_archive zip;
   ZeroMemory(&zip, sizeof(zip));

   if (!mz_zip_reader_init_file(&zip, zip_utf8, 0))
   {
      MessageBoxW(hwnd, L"Failed to open CBZ archive", file_path, MB_OK | MB_ICONERROR);
      return FALSE;
   }

   mz_uint fileCount = mz_zip_reader_get_num_files(&zip);

   wchar_t status_msg[128];
   swprintf(status_msg, 128, L"Files in archive: %u", fileCount);
   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", status_msg);

   for (mz_uint i = 0; i < fileCount; ++i)
   {
      mz_zip_archive_file_stat stat;
      if (!mz_zip_reader_file_stat(&zip, i, &stat))
         continue;

      if (mz_zip_reader_is_file_a_directory(&zip, i))
         continue;

      char relpath_utf8[MAX_PATH];
      mz_zip_reader_get_filename(&zip, i, relpath_utf8, MAX_PATH);

      wchar_t relpath_wide[MAX_PATH];
      MultiByteToWideChar(CP_UTF8, 0, relpath_utf8, -1, relpath_wide, MAX_PATH);

      swprintf(status_msg, 128, L"[%u/%u] %s", i + 1, fileCount, relpath_wide);
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", status_msg);

      wchar_t fullDestW[MAX_PATH];
      swprintf(fullDestW, MAX_PATH, L"%s\\%s", baseFolder, relpath_wide);

      // Recursively create folders in fullDestW
      wchar_t tempPath[MAX_PATH];
      wcscpy(tempPath, fullDestW);
      wchar_t *p = wcschr(tempPath + wcslen(baseFolder) + 1, L'\\');
      while (p)
      {
         *p = L'\0';
         CreateDirectoryW(tempPath, NULL);
         *p = L'\\';
         p = wcschr(p + 1, L'\\');
      }

      char fullDest_utf8[MAX_PATH];
      WideCharToMultiByte(CP_UTF8, 0, fullDestW, -1, fullDest_utf8, MAX_PATH, NULL, NULL);

      if (!mz_zip_reader_extract_to_file(&zip, i, fullDest_utf8, 0))
      {
         swprintf(status_msg, 256, L"❌ Failed to extract: %s", relpath_wide);
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", status_msg);
      }
   }

   mz_zip_reader_end(&zip);

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"📂 Flattening image folders...");
   flatten_and_clean_folder(baseFolder, baseFolder);

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"✅ CBZ extraction complete.");
   wcscpy(final_dir, baseFolder);
   return TRUE;
}

// Create CBZ Archive using miniz
BOOL create_cbz_with_miniz(HWND hwnd, const wchar_t *folder, const wchar_t *output_cbz)
{
   mz_zip_archive zip;
   memset(&zip, 0, sizeof(zip));
   wchar_t status_buf[256];

   // Convert output CBZ filename to UTF-8 for MiniZ
   char output_cbz_utf8[MAX_PATH];
   WideCharToMultiByte(CP_UTF8, 0, output_cbz, -1, output_cbz_utf8, MAX_PATH, NULL, NULL);

   if (!mz_zip_writer_init_file(&zip, output_cbz_utf8, 0))
      return FALSE;

   wchar_t search_path[MAX_PATH];
   swprintf(search_path, MAX_PATH, L"%s\\*", folder);

   WIN32_FIND_DATAW ffd;
   HANDLE hFind = FindFirstFileW(search_path, &ffd);
   if (hFind == INVALID_HANDLE_VALUE)
      return FALSE;

   do
   {
      if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
         wchar_t filepathW[MAX_PATH], archiveW[MAX_PATH];
         swprintf(filepathW, MAX_PATH, L"%s\\%s", folder, ffd.cFileName);
         wcscpy(archiveW, ffd.cFileName);

         // Convert both paths to UTF-8 for MiniZ
         char filepath_utf8[MAX_PATH], archive_utf8[MAX_PATH];
         WideCharToMultiByte(CP_UTF8, 0, filepathW, -1, filepath_utf8, MAX_PATH, NULL, NULL);
         WideCharToMultiByte(CP_UTF8, 0, archiveW, -1, archive_utf8, MAX_PATH, NULL, NULL);

         swprintf(status_buf, 256, L"Adding %s", ffd.cFileName);
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", status_buf);

         if (!mz_zip_writer_add_file(&zip, archive_utf8, filepath_utf8, NULL, 0, MZ_BEST_COMPRESSION))
         {
            mz_zip_writer_end(&zip);
            FindClose(hFind);
            return FALSE;
         }
      }
   } while (FindNextFileW(hFind, &ffd));

   FindClose(hFind);
   mz_zip_writer_finalize_archive(&zip);
   mz_zip_writer_end(&zip);

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", L"Archive finalized successfully.");

   if (g_config.OUTPUT_FOLDER[0] != L'\0')
   {
      const wchar_t *cbz_name = wcsrchr(output_cbz, L'\\');
      cbz_name = cbz_name ? cbz_name + 1 : output_cbz;

      wchar_t dest_cbzW[MAX_PATH];
      swprintf(dest_cbzW, MAX_PATH, L"%s\\%s", g_config.OUTPUT_FOLDER, cbz_name);

      MoveFileW(output_cbz, dest_cbzW);
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"MiniZ: ", L"✔ Archive moved to OUTPUT_FOLDER.");
   }
   else
   {
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
   wchar_t *ext = wcsrchr(cleanName, L'.');
   if (ext && wcscmp(ext, L".cbr") == 0)
      *ext = L'\0';

   swprintf(zip_file, MAX_PATH, L"%s.zip", cleanName);
   swprintf(cbz_file, MAX_PATH, L"%s.cbz", cleanName);

   if (wcslen(g_config.SEVEN_ZIP_PATH) > 0)
   {
      DWORD attrib = GetFileAttributesW(g_config.SEVEN_ZIP_PATH);
      if (!(attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY)))
      {
         if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
            goto fallback;

         STARTUPINFOW si = {sizeof(STARTUPINFOW)};
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