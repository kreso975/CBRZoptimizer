#include "image_handle.h"
#include "gui.h"
#include "resource.h"
#include "functions.h"           // For SendStatus, IMAGE_QUALITY, etc.

#include <windows.h>
#include <shlobj.h>  // For SHFileOperation
#include <shlwapi.h> // If not already present
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>
#include <wchar.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"

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
         task->target_height = _wtoi(IMAGE_SIZE_HEIGHT);

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

   TrimTrailingWhitespace(g_config.IMAGEMAGICK_PATH);

   // Fallback early if IMAGEMAGICK_PATH is missing or invalid
   if (wcslen(g_config.IMAGEMAGICK_PATH) == 0 || GetFileAttributesW(g_config.IMAGEMAGICK_PATH) == INVALID_FILE_ATTRIBUTES ||
       (GetFileAttributesW(g_config.IMAGEMAGICK_PATH) & FILE_ATTRIBUTE_DIRECTORY))
   {
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Falling back to STB optimizer...");
      OutputDebugStringW(g_config.runImageOptimizer ? L"ImageOptimizer1: ON\n" : L"ImageOptimizer1: OFF\n");
      return fallback_optimize_images(hwnd, image_folder);
   }

   const wchar_t *exts[] = {L"jpg", L"png"};
   OutputDebugStringW(g_config.runImageOptimizer ? L"ImageOptimizer: ON\n" : L"ImageOptimizer: OFF\n");
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
               g_config.IMAGEMAGICK_PATH, IMAGE_SIZE_HEIGHT, IMAGE_QUALITY, image_folder, exts[i]);

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