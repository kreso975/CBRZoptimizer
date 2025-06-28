#ifndef UNICODE
#define UNICODE
#endif

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

#include "functions.h"
#include "gui.h"
#include "resource.h"
#include "rar_handle.h"
#include "zip_handle.h"
#include "image_handle.h"
#include "debug.h"


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

// Helper to validate if WINRAR_PATH is set and points to winrar.exe
BOOL is_valid_winrar()
{
   const wchar_t *exe = wcsrchr(g_config.WINRAR_PATH, L'\\');
   return wcslen(g_config.WINRAR_PATH) > 0 &&
          GetFileAttributesW(g_config.WINRAR_PATH) != INVALID_FILE_ATTRIBUTES &&
          exe && _wcsicmp(exe + 1, L"winrar.exe") == 0;
}

// Helper to clean filename (strip path + .cbr/.cbz)
void get_clean_name(const wchar_t *file_path, wchar_t *base)
{
   const wchar_t *file_name = wcsrchr(file_path, L'\\');
   file_name = file_name ? file_name + 1 : file_path;
   wcscpy(base, file_name);
   wchar_t *ext = wcsrchr(base, L'.');
   if (ext && (_wcsicmp(ext, L".cbr") == 0 || _wcsicmp(ext, L".cbz") == 0))
      *ext = L'\0';
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
         if (ext && (_wcsicmp(ext, L".jpg") == 0 || _wcsicmp(ext, L".jpeg") == 0 || _wcsicmp(ext, L".png") == 0  || _wcsicmp(ext, L".bmp") == 0))
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
   if (!path || wcslen(path) == 0)
      return;

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
      // You can log or fallback here if deletion failed
      // MessageBoxW(NULL, L"Failed to delete folder.", L"Error", MB_OK | MB_ICONERROR);
   }
}

void process_file(HWND hwnd, const wchar_t *file_path)
{
   wchar_t base[MAX_PATH];
   get_clean_name(file_path, base);

   wchar_t extracted_dir[MAX_PATH], archive_name[MAX_PATH];
   swprintf(extracted_dir, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, base);
   swprintf(archive_name, MAX_PATH, L"%s\\%s", g_config.TMP_FOLDER, base);

   if (g_StopProcessing)
      return;

   ArchiveType type = detect_archive_type(file_path);
   BOOL extracted = FALSE;

   if (type == ARCHIVE_CBR)
   {
      extracted = is_valid_winrar()
                      ? extract_cbr(hwnd, file_path, extracted_dir)
                      : extract_unrar_dll(hwnd, file_path, extracted_dir);
   }
   else if (type == ARCHIVE_CBZ)
   {
      BOOL hasWinRAR = is_valid_winrar() &&
                       wcslen(g_config.WINRAR_PATH) > 0 &&
                       wcsstr(g_config.WINRAR_PATH, L"WinRAR.exe") != NULL;

      BOOL has7zip = wcslen(g_config.SEVEN_ZIP_PATH) > 0 &&
                     GetFileAttributesW(g_config.SEVEN_ZIP_PATH) != INVALID_FILE_ATTRIBUTES &&
                     !(GetFileAttributesW(g_config.SEVEN_ZIP_PATH) & FILE_ATTRIBUTE_DIRECTORY);

      if (hasWinRAR)
      {
         extracted = extract_external_cbz(hwnd, file_path, extracted_dir, EXTERNAL_APP_WINRAR);
      }
      else if (!extracted && has7zip)
      {
         extracted = extract_external_cbz(hwnd, file_path, extracted_dir, EXTERNAL_APP_7ZIP);
      }
      else
      {
         extracted = extract_cbz(hwnd, file_path, extracted_dir); // fall back to built-in
      }
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

   DEBUG_PRINT(g_config.runImageOptimizer ? L"[DEBUG] RunImageOptimizer = TRUE\n" : L"[DEBUG] RunImageOptimizer = FALSE\n");

   // Image optimization
   if (g_config.runImageOptimizer)
   {
      if (wcslen(g_config.IMAGEMAGICK_PATH) == 0 || GetFileAttributesW(g_config.IMAGEMAGICK_PATH) == INVALID_FILE_ATTRIBUTES ||
          (GetFileAttributesW(g_config.IMAGEMAGICK_PATH) & FILE_ATTRIBUTE_DIRECTORY))
      {
         DEBUG_PRINT(g_config.runImageOptimizer ? L"ImageOptimizer3: ON\n" : L"ImageOptimizer3: OFF\n");
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Falling back to STB optimizer...");
         if (!fallback_optimize_images(hwnd, extracted_dir))
            return;
      }
      else
      {
         DEBUG_PRINT(g_config.runImageOptimizer ? L"ImageOptimizer4: ON\n" : L"ImageOptimizer4: OFF\n");
         if (!optimize_images(hwnd, extracted_dir))
            return;
      }

      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Image optimization completed.");
      if (g_StopProcessing)
         return;
   }

   // Output format decision
   if (g_config.runCompressor)
   {
      wchar_t selectedText[32] = L"";
      int selected = (int)SendMessageW(hOutputType, CB_GETCURSEL, 0, 0);
      SendMessageW(hOutputType, CB_GETLBTEXT, (WPARAM)(INT_PTR)selected, (LPARAM)selectedText);

      BOOL useCBR = FALSE;
      if ((_wcsicmp(selectedText, L"CBR") == 0) ||
          (_wcsicmp(selectedText, L"Keep original") == 0 && type == ARCHIVE_CBR))
      {
         useCBR = is_valid_winrar();
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

   // REMOVE EXTRACTED FOLDER
   if (!g_config.keepExtracted)
   {
      delete_folder_recursive(extracted_dir);
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

