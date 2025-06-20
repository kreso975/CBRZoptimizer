// #include "src/miniz/miniz.h"

#include <windows.h>
#include <shlobj.h> // For SHFileOperation
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>

#include "functions.h"
#include "gui.h"
#include "resource.h"
#include "rar_handle.h"
#include "zip_handle.h"

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
   HWND hOutputType = GetDlgItem(hwnd, ID_OUTPUT_TYPE);
   StartProcessing(hwnd, hOutputType, hListBox);

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

void process_file(HWND hwnd, HWND hOutputType, const wchar_t *file_path)
{
   wchar_t extracted_dir[MAX_PATH], archive_name[MAX_PATH];

   const wchar_t *file_name = wcsrchr(file_path, L'\\');
   file_name = file_name ? file_name + 1 : file_path;

   wchar_t base[MAX_PATH];
   wcscpy(base, file_name);
   wchar_t *ext = wcsrchr(base, L'.');
   if (ext && (_wcsicmp(ext, L".cbr") == 0 || _wcsicmp(ext, L".cbz") == 0))
      *ext = L'\0';

   swprintf(extracted_dir, MAX_PATH, L"%s\\%s", TMP_FOLDER, base);
   swprintf(archive_name, MAX_PATH, L"%s\\%s", TMP_FOLDER, base);

   if (g_StopProcessing)
      return;

   ArchiveType type = detect_archive_type(file_path);
   BOOL extracted = FALSE;

   if (type == ARCHIVE_CBR)
   {
      BOOL useExternalTool = FALSE;
      if (wcslen(WINRAR_PATH) > 0)
      {
         DWORD attrib = GetFileAttributesW(WINRAR_PATH);
         if (attrib != INVALID_FILE_ATTRIBUTES)
         {
            const wchar_t *exeName = wcsrchr(WINRAR_PATH, L'\\');
            if (exeName && (wcsicmp(exeName + 1, L"winrar.exe") == 0 || wcsicmp(exeName + 1, L"unrar.exe") == 0))
               useExternalTool = TRUE;
         }
      }

      if (useExternalTool)
      {
         extracted = extract_cbr(hwnd, file_path, extracted_dir);
      }
      else
      {
         extracted = extract_unrar_dll(hwnd, file_path, extracted_dir);
      }
   }
   else if (type == ARCHIVE_CBZ)
   {
      extracted = extract_cbz(hwnd, file_path, extracted_dir);
   }

   if (!extracted)
      return;

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"Unpacking complete.");

   if (g_StopProcessing)
      return;

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

   // 💡 Decide output format based on dropdown and original type
   BOOL useCBR = FALSE;
   int selected = (int)SendMessageW(hOutputType, CB_GETCURSEL, 0, 0);
   wchar_t selectedText[32];
   SendMessageW(hOutputType, CB_GETLBTEXT, selected, (LPARAM)selectedText);

   // Check if WinRAR is present and valid
   const wchar_t *exeName = wcsrchr(WINRAR_PATH, L'\\');
   BOOL hasValidWinRAR = (wcslen(WINRAR_PATH) > 0 &&
                          GetFileAttributesW(WINRAR_PATH) != INVALID_FILE_ATTRIBUTES &&
                          exeName && _wcsicmp(exeName + 1, L"winrar.exe") == 0);

   // Determine whether to use CBR output
   if ((_wcsicmp(selectedText, L"CBR") == 0) || (_wcsicmp(selectedText, L"Keep original") == 0 && type == ARCHIVE_CBR))
   {
      if (hasValidWinRAR)
         useCBR = TRUE;
   }

   if (useCBR)
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

void AddUniqueToListBox(HWND hwndOwner, HWND hListBox, LPCWSTR itemText)
{
   if (!IsWindow(hListBox) || !itemText || !*itemText)
      return;

   int existingIndex = (int)SendMessageW(hListBox, LB_FINDSTRINGEXACT, -1, (LPARAM)itemText);

   if (existingIndex == LB_ERR)
   {
      SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)itemText);
   }
   else
   {
      SendMessageW(hListBox, LB_SETSEL, FALSE, -1);                  // Deselect all
      SendMessageW(hListBox, LB_SETSEL, TRUE, existingIndex);        // Select duplicate
      SendMessageW(hListBox, LB_SETCARETINDEX, existingIndex, TRUE); // Focus on it

      SetForegroundWindow(hwndOwner);
      MessageBeep(MB_ICONWARNING);

      MSGBOXPARAMSW mbp = {0};
      mbp.cbSize = sizeof(MSGBOXPARAMSW);
      mbp.hwndOwner = hwndOwner;
      mbp.lpszText = L"This file is already in the list.";
      mbp.lpszCaption = L"Duplicate Detected";
      mbp.dwStyle = MB_OK | MB_ICONWARNING | MB_APPLMODAL;
      mbp.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
      MessageBoxIndirectW(&mbp);
   }
}

// Process Dragged Files
void ProcessDroppedFiles(HWND hwnd, HWND hListBox, HDROP hDrop)
{
   wchar_t filePath[MAX_PATH];
   UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

   for (UINT i = 0; i < fileCount; i++)
   {
      if (DragQueryFileW(hDrop, i, filePath, MAX_PATH) > 0)
      {
         AddUniqueToListBox(hwnd, hListBox, filePath);
      }
   }

   DragFinish(hDrop);
}

// Start Processing
void StartProcessing(HWND hwnd, HWND hOutputType, HWND hListBox)
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

      process_file(hwnd, hOutputType, file_path);
      SendMessage(hListBox, LB_DELETESTRING, 0, 0);

      processed++;
   }

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"DONE", L"");
   MessageBeep(MB_ICONINFORMATION); // play warning sound
   MessageBoxW(hwnd, L"Processing Complete!", L"Info", MB_OK);
   ShowWindow(hTerminalProcessingLabel, SW_HIDE);
   ShowWindow(hTerminalProcessingText, SW_HIDE);
}

void BrowseFolder(HWND hwnd, wchar_t *targetPath)
{
   BROWSEINFO bi = {0};
   bi.hwndOwner = hwnd;
   bi.lpszTitle = L"Select a folder:";
   bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;

   // Required for BIF_NEWDIALOGSTYLE on older systems
   OleInitialize(NULL);

   LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
   if (pidl != NULL)
   {
      SHGetPathFromIDListW(pidl, targetPath);
      CoTaskMemFree(pidl);
   }

   OleUninitialize();
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

   if (isValid)
   {
      SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"Keep original");
      SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"CBR");

      // 👇 Select "Keep original" if added
      LRESULT index = SendMessageW(hOutputType, CB_FINDSTRINGEXACT, -1, (LPARAM)L"Keep original");
      if (index != CB_ERR)
      {
         SendMessageW(hOutputType, CB_SETCURSEL, index, 0);
      }
   }
   else
   {
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