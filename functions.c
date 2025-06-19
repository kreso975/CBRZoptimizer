#include "functions.h"
#include "gui.h"
#include "resource.h"

#include "src/miniz/miniz.h"

#include <windows.h>
#include <shlobj.h> // For SHFileOperation
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "src/stb/stb_image.h"
#include "src/stb/stb_image_write.h"
#include "src/stb/stb_image_resize2.h"

HBITMAP LoadBMP(const wchar_t *filename)
{
   return (HBITMAP)LoadImageW(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
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

typedef enum
{
   ARCHIVE_UNKNOWN,
   ARCHIVE_CBR,
   ARCHIVE_CBZ
} ArchiveType;

ArchiveType detect_archive_type(const wchar_t *file_path)
{
   const wchar_t *ext = wcsrchr(file_path, L'.');
   if (!ext)
      return ARCHIVE_UNKNOWN;

   ArchiveType type = ARCHIVE_UNKNOWN;

   if (_wcsicmp(ext, L".cbr") == 0 || _wcsicmp(ext, L".rar") == 0)
   {
      if (is_zip_archive(file_path))
         type = ARCHIVE_CBZ; // mislabeled ZIP
      else
         type = ARCHIVE_CBR;
   }
   else if (_wcsicmp(ext, L".cbz") == 0 || _wcsicmp(ext, L".zip") == 0)
   {
      type = ARCHIVE_CBZ;
   }

   return type;
}

BOOL is_zip_archive(const wchar_t *file_path)
{
   FILE *file = _wfopen(file_path, L"rb");
   if (!file)
      return FALSE;

   unsigned char signature[4];
   size_t read = fread(signature, 1, 4, file);
   fclose(file);

   return (read == 4 && signature[0] == 'P' && signature[1] == 'K');
}

void SendStatus(HWND hwnd, UINT messageId, const wchar_t *prefix, const wchar_t *info)
{
   wchar_t buffer[512];
   swprintf(buffer, 512, L"%s%s", prefix, info);
   PostMessageW(hwnd, messageId, 0, (LPARAM)_wcsdup(buffer));
}

void TrimTrailingWhitespace(wchar_t *str)
{
   int len = wcslen(str);
   while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' '))
      str[--len] = '\0';
}

BOOL find_folder_with_images(const wchar_t *basePath, wchar_t *outPath, int depth)
{
   if (depth > 10)
      return FALSE; // prevent runaway recursion

   wchar_t search[MAX_PATH];
   swprintf(search, MAX_PATH, L"%s\\*", basePath);
   // 📌 Track current folder
   MessageBoxW(NULL, basePath, L"Scanning:", MB_OK);

   WIN32_FIND_DATAW ffd;
   HANDLE hFind = FindFirstFileW(search, &ffd);
   if (hFind == INVALID_HANDLE_VALUE)
      return FALSE;

   BOOL found = FALSE;

   do
   {
      if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
         continue;

      wchar_t fullPath[MAX_PATH];
      swprintf(fullPath, MAX_PATH, L"%s\\%s", basePath, ffd.cFileName);

      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         if (find_folder_with_images(fullPath, outPath, depth + 1))
         {
            found = TRUE;
            break;
         }
      }
      else
      {
         const wchar_t *ext = wcsrchr(ffd.cFileName, L'.');
         if (ext && (_wcsicmp(ext, L".jpg") == 0 ||
                     _wcsicmp(ext, L".jpeg") == 0 ||
                     _wcsicmp(ext, L".png") == 0))
         {
            if (depth > 0)
            {
               wcscpy(outPath, basePath);
               // 📌 Confirm image found location
               MessageBoxW(NULL, basePath, L"Found image folder:", MB_OK);
               found = TRUE;
               break;
            }
         }
      }

   } while (FindNextFileW(hFind, &ffd));

   FindClose(hFind);
   return found;
}

void flatten_and_clean_folder(const wchar_t *source, const wchar_t *target)
{
   wchar_t search[MAX_PATH];
   swprintf(search, MAX_PATH, L"%s\\*", source);

   WIN32_FIND_DATAW ffd;
   HANDLE hFind = FindFirstFileW(search, &ffd);
   if (hFind == INVALID_HANDLE_VALUE)
      return;

   do
   {
      if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
         continue;

      wchar_t fullPath[MAX_PATH];
      swprintf(fullPath, MAX_PATH, L"%s\\%s", source, ffd.cFileName);

      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         flatten_and_clean_folder(fullPath, target);
         delete_folder_recursive(fullPath);
      }
      else
      {
         const wchar_t *ext = wcsrchr(ffd.cFileName, L'.');
         if (ext && (_wcsicmp(ext, L".jpg") == 0 || _wcsicmp(ext, L".jpeg") == 0 || _wcsicmp(ext, L".png") == 0))
         {
            wchar_t dest[MAX_PATH];
            swprintf(dest, MAX_PATH, L"%s\\%s", target, ffd.cFileName);
            MoveFileExW(fullPath, dest, MOVEFILE_REPLACE_EXISTING);
         }
      }

   } while (FindNextFileW(hFind, &ffd));

   FindClose(hFind);
}

void delete_folder_recursive(const wchar_t *path)
{
   wchar_t temp[MAX_PATH];
   swprintf(temp, sizeof(temp), L"%s\\", path); // Ensure trailing backslash

   // Double null-terminate the string as required by SHFileOperation
   temp[wcslen(temp) + 1] = L'\0';

   SHFILEOPSTRUCTW op = {0};
   op.wFunc = FO_DELETE;
   op.pFrom = temp;
   op.fFlags = FOF_NO_UI | FOF_SILENT | FOF_NOCONFIRMATION;

   SHFileOperationW(&op);
}

void replace_all(wchar_t *str, const wchar_t *old_sub, const wchar_t *new_sub)
{
   wchar_t buffer[4096];
   wchar_t *pos, *curr = str;
   int old_len = wcslen(old_sub), new_len = wcslen(new_sub);
   buffer[0] = L'\0';

   while ((pos = wcsstr(curr, old_sub)) != NULL)
   {
      wcsncat(buffer, curr, pos - curr);
      wcscat(buffer, new_sub);
      curr = pos + old_len;
   }
   wcscat(buffer, curr);
   wcscpy(str, buffer);
}

BOOL extract_cbz(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir)
{
   wchar_t cleanDir[MAX_PATH], baseFolder[MAX_PATH];
   wcscpy(cleanDir, file_path);

   wchar_t *ext = wcsrchr(cleanDir, L'.');
   if (ext && (_wcsicmp(ext, L".cbz") == 0 || _wcsicmp(ext, L".zip") == 0))
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

typedef struct
{
   wchar_t image_path[MAX_PATH];
   HWND hwnd;
   int target_height;
} ImageTask;

// STB safe write callback
void stb_write_func(void *context, void *data, int size)
{
   FILE *fp = (FILE *)context;
   fwrite(data, 1, size, fp);
}

DWORD WINAPI OptimizeImageThread(LPVOID lpParam)
{
   ImageTask *task = (ImageTask *)lpParam;

   wchar_t *pathW = task->image_path;
   char utf8_path[MAX_PATH];
   WideCharToMultiByte(CP_UTF8, 0, pathW, -1, utf8_path, MAX_PATH, NULL, NULL);

   // Open image with Win32 API
   HANDLE hFile = CreateFileW(pathW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
   {
      SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"❌ Unable to open file.");
      free(task);
      return 1;
   }

   DWORD fileSize = GetFileSize(hFile, NULL);
   BYTE *buffer = malloc(fileSize);
   if (!buffer)
   {
      CloseHandle(hFile);
      SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"❌ Memory allocation failed.");
      free(task);
      return 1;
   }

   DWORD bytesRead;
   ReadFile(hFile, buffer, fileSize, &bytesRead, NULL);
   CloseHandle(hFile);

   int w, h, c;
   unsigned char *input = stbi_load_from_memory(buffer, fileSize, &w, &h, &c, 3);
   free(buffer);

   if (!input)
   {
      SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"❌ Failed to decode image.");
      free(task);
      return 1;
   }

   int newH = task->target_height > 0 ? task->target_height : 1;
   int newW = max(1, w * newH / h);

   unsigned char *resized = malloc(newW * newH * 3);
   if (!resized)
   {
      stbi_image_free(input);
      SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"❌ Memory allocation failed.");
      free(task);
      return 1;
   }

   stbir_resize_uint8_linear(input, w, h, 0, resized, newW, newH, 0, STBIR_RGB);
   stbi_image_free(input);

   const wchar_t *extW = wcsrchr(pathW, L'.');
   int result = 0;

   FILE *fp = _wfopen(pathW, L"wb");
   if (fp)
   {
      if (extW && _wcsicmp(extW, L".jpg") == 0)
      {
         result = stbi_write_jpg_to_func(stb_write_func, fp, newW, newH, 3, resized, _wtoi(IMAGE_QUALITY));
      }
      else if (extW && _wcsicmp(extW, L".png") == 0)
      {
         result = stbi_write_png_to_func(stb_write_func, fp, newW, newH, 3, resized, newW * 3);
      }
      fclose(fp);
   }

   free(resized);

   SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ",
              result ? L"✔ Image optimized and saved." : L"⚠ Failed to write image.");

   free(task);
   return 0;
}

// Fallback Image Optimization using STB
BOOL fallback_optimize_images(HWND hwnd, const wchar_t *folder)
{
   if (g_StopProcessing)
      return FALSE;

   SYSTEM_INFO sysinfo;
   GetSystemInfo(&sysinfo);
   DWORD numCPU = sysinfo.dwNumberOfProcessors;

   DWORD max_threads = max(1, min(numCPU * 2, 64));
   const wchar_t *exts[] = {L"jpg", L"png"};
   HANDLE *threads = malloc(sizeof(HANDLE) * max_threads);
   int thread_count = 0;
   wchar_t search_path[MAX_PATH], image_path[MAX_PATH];

   for (int i = 0; i < 2; i++)
   {
      swprintf(search_path, MAX_PATH, L"%s\\*.%s", folder, exts[i]);
      WIN32_FIND_DATAW ffd;
      HANDLE hFind = FindFirstFileW(search_path, &ffd);
      if (hFind == INVALID_HANDLE_VALUE)
         continue;

      do
      {
         if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

         if (g_StopProcessing)
            break;

         swprintf(image_path, MAX_PATH, L"%s\\%s", folder, ffd.cFileName);

         ImageTask *task = malloc(sizeof(ImageTask));
         if (!task)
            continue;

         wcscpy(task->image_path, image_path);
         task->hwnd = hwnd;
         task->target_height = _wtoi(IMAGE_SIZE);

         threads[thread_count++] = CreateThread(NULL, 0, OptimizeImageThread, task, 0, NULL);

         if (thread_count == max_threads)
         {
            WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
            for (int t = 0; t < thread_count; t++)
               CloseHandle(threads[t]);
            thread_count = 0;
         }

      } while (FindNextFileW(hFind, &ffd));

      FindClose(hFind);
      if (g_StopProcessing)
         break;
   }

   if (thread_count > 0)
   {
      WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
      for (int t = 0; t < thread_count; t++)
         CloseHandle(threads[t]);
   }

   free(threads);

   if (!g_StopProcessing)
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"✅ All formats processed.");
   else
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"🛑 Processing canceled.");

   return !g_StopProcessing;
}

// Optimize Images
BOOL optimize_images(HWND hwnd, const wchar_t *image_folder)
{
   wchar_t command[MAX_PATH], buffer[4096];
   DWORD bytesRead;
   HANDLE hReadPipe, hWritePipe;
   SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
   STARTUPINFOW si = {.cb = sizeof(si)};
   PROCESS_INFORMATION pi;

   TrimTrailingWhitespace(IMAGEMAGICK_PATH);

   // Fallback early if IMAGEMAGICK_PATH is missing or invalid
   if (wcslen(IMAGEMAGICK_PATH) == 0 || GetFileAttributesW(IMAGEMAGICK_PATH) == INVALID_FILE_ATTRIBUTES ||
       (GetFileAttributesW(IMAGEMAGICK_PATH) & FILE_ATTRIBUTE_DIRECTORY))
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Falling back to STB optimizer...");
      return fallback_optimize_images(hwnd, image_folder);
   }

   const wchar_t *exts[] = {L"jpg", L"png"};
   MessageBoxW(hwnd, L"Running external tool: Enter", L"Image optimization", MB_OK | MB_ICONERROR);
   for (int i = 0; i < 2; i++)
   {
      if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
      {
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", L"Pipe creation failed. Using STB fallback.");
         return fallback_optimize_images(hwnd, image_folder);
      }

      si.hStdOutput = hWritePipe;
      si.hStdError = hWritePipe;
      si.wShowWindow = SW_HIDE;
      si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

      swprintf(command, MAX_PATH, L"\"%s\" mogrify -resize %s -quality %s \"%s\\*.%s\"",
               IMAGEMAGICK_PATH, IMAGE_SIZE, IMAGE_QUALITY, image_folder, exts[i]);
      MessageBoxW(hwnd, command, L"Image optimization", MB_OK | MB_ICONERROR);

      if (!CreateProcessW(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
      {
         CloseHandle(hWritePipe);
         CloseHandle(hReadPipe);
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", L"Failed to execute. Falling back to STB.");
         return fallback_optimize_images(hwnd, image_folder);
      }

      CloseHandle(hWritePipe);
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", command);

      if (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
      {
         buffer[bytesRead] = '\0';
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", buffer);
      }
      else
      {
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", L"No output captured.");
      }

      CloseHandle(hReadPipe);
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
   }

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", L"✔ All formats optimized.");
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

   if (OUTPUT_FOLDER[0] != L'\0')
   {
      const wchar_t *cbz_name = wcsrchr(output_cbz, L'\\');
      cbz_name = cbz_name ? cbz_name + 1 : output_cbz;

      wchar_t dest_cbzW[MAX_PATH];
      swprintf(dest_cbzW, MAX_PATH, L"%s\\%s", OUTPUT_FOLDER, cbz_name);

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

   if (wcslen(SEVEN_ZIP_PATH) > 0)
   {
      DWORD attrib = GetFileAttributesW(SEVEN_ZIP_PATH);
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
                  SEVEN_ZIP_PATH, zip_file, image_folder);

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

            if (OUTPUT_FOLDER[0] != L'\0')
            {
               const wchar_t *cbz_name = wcsrchr(cbz_file, L'\\');
               cbz_name = cbz_name ? cbz_name + 1 : cbz_file;

               wchar_t dest_cbz[MAX_PATH];
               swprintf(dest_cbz, MAX_PATH, L"%s\\%s", OUTPUT_FOLDER, cbz_name);
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

void process_file(HWND hwnd, const wchar_t *file_path)
{
   wchar_t extracted_dir[512], archive_name[512];

   swprintf(extracted_dir, MAX_PATH, L"D:\\Downloads\\TMPVideo\\%s", wcsrchr(file_path, L'\\') + 1);
   swprintf(archive_name, MAX_PATH, L"D:\\Downloads\\TMPVideo\\%s", wcsrchr(file_path, L'\\') + 1);

   if (g_StopProcessing)
      return;

   ArchiveType type = detect_archive_type(file_path);
   BOOL extracted = FALSE;

   if (type == ARCHIVE_CBR)
      extracted = extract_cbr(hwnd, file_path, extracted_dir);
   else if (type == ARCHIVE_CBZ)
      extracted = extract_cbr(hwnd, file_path, extracted_dir);

   if (!extracted)
      return;

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"Unpacking complete.");

   // Image optimization
   if (g_StopProcessing)
      return;
   // Fallback early if IMAGEMAGICK_PATH is missing or invalid
   if (wcslen(IMAGEMAGICK_PATH) == 0 || GetFileAttributesW(IMAGEMAGICK_PATH) == INVALID_FILE_ATTRIBUTES ||
       (GetFileAttributesW(IMAGEMAGICK_PATH) & FILE_ATTRIBUTE_DIRECTORY))
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Falling back to STB optimizer...");
      if (!fallback_optimize_images(hwnd, extracted_dir))
         return;
   }
   else
   {
      if (!optimize_images(hwnd, extracted_dir))
         return;
   }
   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Image optimization completed.");

   if (g_StopProcessing)
      return;
   if (!create_cbz_archive(hwnd, extracted_dir, archive_name))
      return;
   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"7-Zip: ", L"Completed");
}

// Process Dragged Files
void ProcessDroppedFiles(HWND hListBox, HDROP hDrop)
{
   wchar_t filePath[MAX_PATH];
   UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

   for (UINT i = 0; i < fileCount; i++)
   {
      if (DragQueryFileW(hDrop, i, filePath, MAX_PATH) > 0)
      {
         if (IsWindow(hListBox))
         {
            SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)filePath);
         }
      }
   }

   DragFinish(hDrop);
}

// Start Processing
void StartProcessing(HWND hwnd, HWND hListBox)
{
   ShowWindow(hTerminalProcessingLabel, SW_SHOW);
   ShowWindow(hTerminalProcessingText, SW_SHOW);

   int total = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
   int processed = 0;

   int count;
   while (!g_StopProcessing && (count = SendMessage(hListBox, LB_GETCOUNT, 0, 0)) > 0)
   {
      wchar_t file_path[MAX_PATH];
      SendMessageW(hListBox, LB_GETTEXT, 0, (LPARAM)file_path);

      if (g_StopProcessing)
      {
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"CANCELLED", L"");
         MessageBoxW(hwnd, L"Processing was canceled.", L"Canceled", MB_OK | MB_ICONEXCLAMATION);
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
   MessageBoxW(hwnd, L"Processing Complete!", L"Info", MB_OK);
   ShowWindow(hTerminalProcessingLabel, SW_HIDE);
   ShowWindow(hTerminalProcessingText, SW_HIDE);
}

void BrowseFolder(HWND hwnd, wchar_t *targetPath)
{
   BROWSEINFO bi = {0};
   bi.hwndOwner = hwnd;
   bi.lpszTitle = "Select a folder:";
   LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
   if (pidl != NULL)
   {
      SHGetPathFromIDListW(pidl, targetPath);
   }
}

void BrowseFile(HWND hwnd, wchar_t *targetPath)
{
   OPENFILENAMEW ofn;
   ZeroMemory(&ofn, sizeof(ofn));
   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFile = targetPath;
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrFilter = L"Executables\0*.exe\0All Files\0*.*\0";
   ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
   GetOpenFileNameW(&ofn);
}

// File Dialog with Multi-Selection
void OpenFileDialog(HWND hwnd, HWND hListBox)
{
   OPENFILENAMEW ofn;
   wchar_t fileNames[MAX_PATH * 50] = {0}; // Large buffer to hold multiple file paths

   ZeroMemory(&ofn, sizeof(ofn));
   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = L"CBR/CBZ/RAR/ZIP Files (*.cbr;*.cbz;*.rar;*.zip)\0*.cbr;*.cbz;*.rar;*.zip\0All Files\0*.*\0";
   ofn.lpstrFile = fileNames;
   ofn.nMaxFile = sizeof(fileNames);
   ofn.Flags = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

   if (GetOpenFileNameW(&ofn))
   {
      wchar_t *p = fileNames + wcslen(fileNames) + 1;

      if (*p == '\0')
      {
         // **Single file selected**
         SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)fileNames);
      }
      else
      {
         // **Multiple files selected**
         wchar_t folder[MAX_PATH];
         wcscpy(folder, fileNames);

         while (*p)
         {
            wchar_t fullPath[MAX_PATH];
            swprintf(fullPath, sizeof(fullPath), L"%s\\%s", folder, p); // Safer than sprintf
            SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)fullPath);
            p += wcslen(p) + 1;
         }
      }
   }
}

void RemoveSelectedItems(HWND hListBox)
{
   int selectedCount = SendMessageW(hListBox, LB_GETSELCOUNT, 0, 0); // Get number of selected items

   if (selectedCount > 0)
   {
      int *selectedIndexes = (int *)malloc(selectedCount * sizeof(int));              // Allocate memory
      SendMessageW(hListBox, LB_GETSELITEMS, selectedCount, (LPARAM)selectedIndexes); // Get indexes

      for (int i = selectedCount - 1; i >= 0; i--)
      {
         SendMessageW(hListBox, LB_DELETESTRING, selectedIndexes[i], 0);
      }

      free(selectedIndexes); // Free memory
   }
}

void update_output_type_dropdown(HWND hOutputType, const wchar_t *winrarPath)
{
    const wchar_t *filename = wcsrchr(winrarPath, L'\\');
    filename = filename ? filename + 1 : winrarPath;

    BOOL isValid = 
        wcslen(winrarPath) > 0 &&
        GetFileAttributesW(winrarPath) != INVALID_FILE_ATTRIBUTES &&
        !(GetFileAttributesW(winrarPath) & FILE_ATTRIBUTE_DIRECTORY) &&
        _wcsicmp(filename, L"winrar.exe") == 0;

    SendMessageW(hOutputType, CB_RESETCONTENT, 0, 0);
    SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"CBZ");

    if (isValid) {
        SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"Keep original");
        SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"CBR");

        // 👇 Select "Keep original" if added
        LRESULT index = SendMessageW(hOutputType, CB_FINDSTRINGEXACT, -1, (LPARAM)L"Keep original");
        if (index != CB_ERR) {
            SendMessageW(hOutputType, CB_SETCURSEL, index, 0);
        }
    } else {
        // No WinRAR, so select "CBZ" which is index 0
        SendMessageW(hOutputType, CB_SETCURSEL, 0, 0);
    }
}


void load_config_values(HWND hTmpFolder, HWND hOutputFolder, HWND hWinrarPath,
                        HWND hSevenZipPath, HWND hImageMagickPath, HWND hImageDpi,
                        HWND hImageSize, HWND hImageQualityValue, HWND hImageQualitySlider,
                        HWND hOutputRunExtract, HWND hOutputRunImageOptimizer,
                        HWND hOutputRunCompressor, HWND hOutputKeepExtracted, HWND hOutputType,
                        int controlCount)
{
   wchar_t buffer[256];

   // 🔍 Construct full path to config.ini
   wchar_t iniPath[MAX_PATH];
   GetModuleFileNameW(NULL, iniPath, MAX_PATH);
   wchar_t *lastSlash = wcsrchr(iniPath, L'\\');
   if (lastSlash)
      *(lastSlash + 1) = L'\0'; // Trim to folder path
   wcscat(iniPath, L"config.ini");

   // Paths
   GetPrivateProfileStringW(L"Paths", L"TMP_FOLDER", L"", buffer, sizeof(buffer), iniPath);
   SetWindowTextW(hTmpFolder, buffer);
   wcscpy(TMP_FOLDER, buffer);

   GetPrivateProfileStringW(L"Paths", L"OUTPUT_FOLDER", L"", buffer, sizeof(buffer), iniPath);
   SetWindowTextW(hOutputFolder, buffer);
   wcscpy(OUTPUT_FOLDER, buffer);

   GetPrivateProfileStringW(L"Paths", L"WINRAR_PATH", L"", buffer, sizeof(buffer), iniPath);
   SetWindowTextW(hWinrarPath, buffer);
   wcscpy(WINRAR_PATH, buffer);

   GetPrivateProfileStringW(L"Paths", L"SEVEN_ZIP_PATH", L"", buffer, sizeof(buffer), iniPath);
   SetWindowTextW(hSevenZipPath, buffer);
   wcscpy(SEVEN_ZIP_PATH, buffer);

   GetPrivateProfileStringW(L"Paths", L"IMAGEMAGICK_PATH", L"", buffer, sizeof(buffer), iniPath);
   SetWindowTextW(hImageMagickPath, buffer);
   wcscpy(IMAGEMAGICK_PATH, buffer);

   // Image
   GetPrivateProfileStringW(L"Image", L"IMAGE_DPI", L"", buffer, sizeof(buffer), iniPath);
   SetWindowTextW(hImageDpi, buffer);
   wcscpy(IMAGE_DPI, buffer);

   GetPrivateProfileStringW(L"Image", L"IMAGE_SIZE", L"", buffer, sizeof(buffer), iniPath);
   SetWindowTextW(hImageSize, buffer);
   wcscpy(IMAGE_SIZE, buffer);

   // Image Quality
   GetPrivateProfileStringW(L"Image", L"IMAGE_QUALITY", L"35", buffer, sizeof(buffer), iniPath);
   SendMessageW(hImageQualitySlider, TBM_SETPOS, TRUE, _wtoi(buffer));

   // Also update the label showing the current image quality
   SetWindowTextW(hImageQualityValue, buffer);
   wcscpy(IMAGE_QUALITY, buffer); // Optional: keep IMAGE_QUALITY in sync

   // Output options
   for (int i = 0; i < controlCount; ++i)
   {
      GetPrivateProfileStringW(L"Output", controls[i].configKey, L"false", buffer, sizeof(buffer), iniPath);
      SendMessageW(*controls[i].hCheckbox, BM_SETCHECK,
                   (wcscmp(buffer, L"true") == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
   }

   // Output type dropdown behavior
   update_output_type_dropdown(hOutputType, WINRAR_PATH);
}