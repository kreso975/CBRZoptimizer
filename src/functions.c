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

HBITMAP LoadBMP(const wchar_t *filename)
{
   return (HBITMAP)LoadImageW(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
}

// Disable all controls in the "FilesGroup"
// EnableGroupElements(L"FilesGroup", FALSE);

// Later, re-enable them
// EnableGroupElements(L"FilesGroup", TRUE);

void EnableGroupElements(LPCWSTR groupName, BOOL enable)
{
   for (int i = 0; i < groupElementsCount; ++i)
   {
      if (wcscmp(groupElements[i].group, groupName) == 0 && *groupElements[i].hwndPtr)
      {
         EnableWindow(*groupElements[i].hwndPtr, enable);
      }
   }
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

void ValidateAndSaveInput(HWND changedControl, const wchar_t *iniPath)
{
   for (size_t i = 0; i < inputsCount; ++i)
   {
      if (inputs[i].hEdit && *(inputs[i].hEdit) == changedControl)
      {
         wchar_t buffer[MAX_PATH] = {0};
         GetWindowTextW(changedControl, buffer, MAX_PATH);

         if (!inputs[i].defaultText || wcscmp(buffer, inputs[i].defaultText) == 0)
            return;

         // Keys that may accept empty values without folder validation
         BOOL allowEmpty = (wcscmp(inputs[i].configKey, L"WINRAR_PATH") == 0 ||
                            wcscmp(inputs[i].configKey, L"SEVEN_ZIP_PATH") == 0 ||
                            wcscmp(inputs[i].configKey, L"IMAGEMAGICK_PATH") == 0);

         if (wcslen(buffer) == 0 && allowEmpty)
         {
            inputs[i].defaultText[0] = L'\0';
            WritePrivateProfileStringW(inputs[i].configSection, inputs[i].configKey, L"", iniPath); // preserve key, empty value
            return;
         }

         // Folder must exist for non-empty inputs
         DWORD attrs = GetFileAttributesW(buffer);
         if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
         {
            wchar_t msg[512];
            swprintf(msg, 512, L"The folder \"%s\" does not exist.\n\nThe previous value will be kept.", buffer);
            MessageBoxW(changedControl, msg, L"Invalid Folder", MB_OK | MB_ICONWARNING);
            SetWindowTextW(changedControl, inputs[i].defaultText);
            return;
         }

         wcscpy(inputs[i].defaultText, buffer);
         WritePrivateProfileStringW(inputs[i].configSection, inputs[i].configKey, buffer, iniPath);
         return;
      }
   }
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


void SendStatus(HWND hwnd, UINT messageId, const wchar_t *prefix, const wchar_t *info)
{
   wchar_t buffer[512];
   swprintf(buffer, 512, L"%s%s", prefix, info);
   PostMessageW(hwnd, messageId, 0, (LPARAM)_wcsdup(buffer));
}

void TrimTrailingWhitespace(wchar_t *str)
{
   size_t len = wcslen(str);
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
   // MessageBoxW(NULL, basePath, L"Scanning:", MB_OK);

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
               // MessageBoxW(NULL, basePath, L"Found image folder:", MB_OK);
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

void process_file(HWND hwnd, HWND hOutputType, const wchar_t *file_path)
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
      OutputDebugStringW(L"Extracting Failed!");
      SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"Failed!");
      return;
   }

   SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Extracting: ", L"Unpacking complete.");
   if (g_StopProcessing)
      return;

   OutputDebugStringW(g_config.runImageOptimizer ? L"[DEBUG] RunImageOptimizer = TRUE\n" : L"[DEBUG] RunImageOptimizer = FALSE\n");

   // Image optimization
   if (g_config.runImageOptimizer)
   {
      if (wcslen(g_config.IMAGEMAGICK_PATH) == 0 ||
          GetFileAttributesW(g_config.IMAGEMAGICK_PATH) == INVALID_FILE_ATTRIBUTES ||
          (GetFileAttributesW(g_config.IMAGEMAGICK_PATH) & FILE_ATTRIBUTE_DIRECTORY))
      {
         OutputDebugStringW(g_config.runImageOptimizer ? L"ImageOptimizer3: ON\n" : L"ImageOptimizer3: OFF\n");
         SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Falling back to STB optimizer...");
         if (!fallback_optimize_images(hwnd, extracted_dir))
            return;
      }
      else
      {
         OutputDebugStringW(g_config.runImageOptimizer ? L"ImageOptimizer4: ON\n" : L"ImageOptimizer4: OFF\n");
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
void StartProcessing(HWND hwnd, HWND hOutputType, HWND hListBox)
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

      process_file(hwnd, hOutputType, file_path);
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

void AddUniqueToListBox(HWND hwndOwner, HWND hListBox, LPCWSTR itemText)
{
   OutputDebugStringW(itemText);
   if (!IsWindow(hListBox) || !itemText || !*itemText)
      return;

   int existingIndex = (int)SendMessageW(hListBox, LB_FINDSTRINGEXACT, (WPARAM)(INT_PTR)-1, (LPARAM)itemText);

   if (existingIndex == LB_ERR)
   {

      SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)itemText);
   }
   else
   {
      SendMessageW(hListBox, LB_SETSEL, FALSE, -1);                                   // Deselect all
      SendMessageW(hListBox, LB_SETSEL, TRUE, existingIndex);                         // Select duplicate
      SendMessageW(hListBox, LB_SETCARETINDEX, (WPARAM)(INT_PTR)existingIndex, TRUE); // Focus on it

      SetForegroundWindow(hwndOwner);
      MessageBeep(MB_ICONWARNING);
      MessageBoxCentered(hwndOwner, L"This file is already in the list.", L"Duplicate Detected", MB_OK | MB_ICONWARNING);
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

void RemoveSelectedItems(HWND hListBox)
{
   LRESULT result = SendMessageW(hListBox, LB_GETSELCOUNT, 0, 0); // Get number of selected items
   int selectedCount = (int)result;                               // with bounds check if needed

   if (selectedCount > 0)
   {
      size_t allocSize = (size_t)selectedCount * sizeof(int);
      int *selectedIndexes = (int *)malloc(allocSize);                                                 // Allocate memory
      SendMessageW(hListBox, LB_GETSELITEMS, (WPARAM)(INT_PTR)selectedCount, (LPARAM)selectedIndexes); // Get indexes

      for (int i = selectedCount - 1; i >= 0; i--)
      {
         SendMessageW(hListBox, LB_DELETESTRING, (WPARAM)(INT_PTR)selectedIndexes[i], 0);
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
      LRESULT index = SendMessageW(hOutputType, CB_FINDSTRINGEXACT, (WPARAM)(INT_PTR)-1, (LPARAM)L"Keep original");
      if (index != CB_ERR)
      {
         SendMessageW(hOutputType, CB_SETCURSEL, (WPARAM)index, 0);
      }
   }
   else
   {
      // No WinRAR, so select "CBZ" which is index 0
      SendMessageW(hOutputType, CB_SETCURSEL, 0, 0);
   }
}