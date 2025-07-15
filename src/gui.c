#ifndef UNICODE
#define UNICODE
#endif
#define _WIN32_IE 0x0400    // Or higher like 0x0600 if needed
#define _WIN32_WINNT 0x0501 // Or match your MinGW target

#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h> // Slider control, etc.
#include <wchar.h>
#include "window.h"
#include "gui.h"
#include "resource.h"
#include "functions.h"
#include "debug.h"

EditBrowseControl inputs[] = {
    {L"Temp Folder:", L"TMP_FOLDER", L"Paths", g_config.TMP_FOLDER, 30, 100, 200, ID_TMP_FOLDER_BROWSE, &hTmpFolderLabel, &hTmpFolder, &hTmpBrowse, &hButtonBrowse, NULL, L"Select temporary folder for processing"},
    {L"Output Folder:", L"OUTPUT_FOLDER", L"Paths", g_config.OUTPUT_FOLDER, 60, 100, 200, ID_OUTPUT_FOLDER_BROWSE, &hOutputFolderLabel, &hOutputFolder, &hOutputBrowse, &hButtonBrowse, NULL, L"Select Output folder for copying"},
    {L"WinRAR Path:", L"WINRAR_PATH", L"Paths", g_config.WINRAR_PATH, 90, 100, 200, ID_WINRAR_PATH_BROWSE, &hWinrarLabel, &hWinrarPath, &hWinrarBrowse, &hButtonBrowse, NULL, L"Select WinRar.exe"},
    {L"7-Zip Path:", L"SEVEN_ZIP_PATH", L"Paths", g_config.SEVEN_ZIP_PATH, 120, 100, 200, ID_SEVEN_ZIP_PATH_BROWSE, &hSevenZipLabel, &hSevenZipPath, &hSevenZipBrowse, &hButtonBrowse, NULL, L"Select 7z.exe"},
    {L"ImageMagick:", L"IMAGEMAGICK_PATH", L"Paths", g_config.IMAGEMAGICK_PATH, 150, 100, 200, ID_IMAGEMAGICK_PATH_BROWSE, &hImageMagickLabel, &hImageMagickPath, &hImageMagickBrowse, &hButtonBrowse, NULL, L"Select ImageMagick.exe"},
    {L"MuPDF Tool:", L"MUTOOL_PATH", L"Paths", g_config.MUTOOL_PATH, 180, 100, 200, ID_MUTOOL_PATH_BROWSE, &hMuToolLabel, &hMuToolPath, &hMuToolBrowse, &hButtonBrowse, NULL, L"Select mutool.exe"}};

const size_t inputsCount = sizeof(inputs) / sizeof(inputs[0]);

LabelCheckboxPair controls[] = {
    {L"Image optimization", L"hOutputRunImageOptimizer", L"Output", 450, &hOutputRunImageOptimizer, &hOutputRunImageOptimizerLabel, &g_config.runImageOptimizer},
    {L"Compress folder", L"hOutputRunCompressor", L"Output", 470, &hOutputRunCompressor, &hOutputRunCompressorLabel, &g_config.runCompressor},
    {L"Keep extracted folders", L"hOutputKeepExtracted", L"Output", 490, &hOutputKeepExtracted, &hOutputKeepExtractedLabel, &g_config.keepExtracted},

    {L"Resize image:", L"IMAGE_RESIZE_TO", L"Image", 490, &hImageResizeTo, &hImageResizeToLabel, &g_config.resizeTo},
    {L"Keep Aspect Ratio", L"IMAGE_KEEP_ASPECT_RATIO", L"Image", 490, &hImageKeepAspectRatio, &hImageKeepAspectRatioLabel, &g_config.keepAspectRatio},
    {L"Allow upscaling", L"IMAGE_ALLOW_UPSCALING", L"Image", 490, &hImageAllowUpscaling, &hImageAllowUpscalingLabel, &g_config.allowUpscaling}};

const int controlCount = sizeof(controls) / sizeof(controls[0]);

void SendStatus(HWND hwnd, UINT messageId, const wchar_t *prefix, const wchar_t *info)
{
   wchar_t buffer[512];
   swprintf(buffer, 512, L"%s%s", prefix, info);
   PostMessageW(hwnd, messageId, 0, (LPARAM)_wcsdup(buffer));
}

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

// EnableGroupElements(L"FilesGroup", FALSE);
// EnableGroupElements(L"FilesGroup", TRUE);
void EnableResizeGroupWithLogic(LPCWSTR groupName, BOOL enable)
{
   // 1. Handle "Resize To" checkbox and label independently
   BOOL forceEnableResizeToggle = enable && g_config.runImageOptimizer;
   EnableWindow(hImageResizeToLabel, forceEnableResizeToggle);
   EnableWindow(hImageResizeTo, forceEnableResizeToggle);

   // 2. If disabling the group, turn off all other group controls (except the two above)
   if (!enable)
   {
      for (int i = 0; i < groupElementsCount; ++i)
      {
         if (wcscmp(groupElements[i].group, groupName) == 0 && *groupElements[i].hwndPtr)
         {
            HWND h = *groupElements[i].hwndPtr;
            if (h != hImageResizeTo && h != hImageResizeToLabel)
            {
               EnableWindow(h, FALSE);
            }
         }
      }
      return;
   }

   // 3. General enable for non-special group controls
   for (int i = 0; i < groupElementsCount; ++i)
   {
      if (wcscmp(groupElements[i].group, groupName) == 0 && *groupElements[i].hwndPtr)
      {
         HWND h = *groupElements[i].hwndPtr;
         if (h != hImageResizeTo && h != hImageResizeToLabel)
         {
            EnableWindow(h, TRUE);
         }
      }
   }

   // 4. Toggle advanced controls based on resize settings
   EnableWindow(hImageKeepAspectRatioLabel, g_config.resizeTo);
   EnableWindow(hImageKeepAspectRatio, g_config.resizeTo);
   EnableWindow(hImageAllowUpscalingLabel, g_config.resizeTo);
   EnableWindow(hImageAllowUpscaling, g_config.resizeTo);

   DEBUG_PRINT(g_config.resizeTo ? L"[DEBUG] ResizeTo = TRUE\n" : L"[DEBUG] ResizeTo = FALSE\n");

   if (!g_config.resizeTo)
   {
      DEBUG_PRINT(L"[DEBUG] ResizeTo is OFF → disabling dimensions\n");
      EnableWindow(hImageSizeWidthLabel, FALSE);
      EnableWindow(hImageSizeWidth, FALSE);
      EnableWindow(hImageSizeHeightLabel, FALSE);
      EnableWindow(hImageSizeHeight, FALSE);
      return;
   }

   if (g_config.keepAspectRatio)
   {
      DEBUG_PRINT(L"[DEBUG] KeepAspectRatio = TRUE\n");

      if (wcscmp(g_config.IMAGE_TYPE, L"Portrait") == 0)
      {
         DEBUG_PRINT(L"[DEBUG] Portrait mode\n");
         EnableWindow(hImageSizeWidthLabel, FALSE);
         EnableWindow(hImageSizeWidth, FALSE);
         EnableWindow(hImageSizeHeightLabel, TRUE);
         EnableWindow(hImageSizeHeight, TRUE);
      }
      else if (wcscmp(g_config.IMAGE_TYPE, L"Landscape") == 0)
      {
         DEBUG_PRINT(L"[DEBUG] Landscape mode\n");
         EnableWindow(hImageSizeWidthLabel, TRUE);
         EnableWindow(hImageSizeWidth, TRUE);
         EnableWindow(hImageSizeHeightLabel, FALSE);
         EnableWindow(hImageSizeHeight, FALSE);
      }
      else
      {
         DEBUG_PRINT(L"[DEBUG] Unknown orientation — disabling both\n");
         EnableWindow(hImageSizeWidthLabel, FALSE);
         EnableWindow(hImageSizeWidth, FALSE);
         EnableWindow(hImageSizeHeightLabel, FALSE);
         EnableWindow(hImageSizeHeight, FALSE);
      }
   }
   else
   {
      DEBUG_PRINT(L"[DEBUG] KeepAspectRatio = FALSE → enabling both dimensions\n");
      EnableWindow(hImageSizeWidthLabel, TRUE);
      EnableWindow(hImageSizeWidth, TRUE);
      EnableWindow(hImageSizeHeightLabel, TRUE);
      EnableWindow(hImageSizeHeight, TRUE);
   }
}

void ValidateAndSaveInput(HWND hwnd, HWND changedControl, const wchar_t *iniPath)
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
                            wcscmp(inputs[i].configKey, L"IMAGEMAGICK_PATH") == 0 ||
                            wcscmp(inputs[i].configKey, L"MUTOOL_PATH") == 0);

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
            MessageBoxCentered(hwnd, msg, L"Invalid Folder", MB_OK | MB_ICONWARNING);
            SetWindowTextW(changedControl, inputs[i].defaultText);
            return;
         }

         wcscpy(inputs[i].defaultText, buffer);
         WritePrivateProfileStringW(inputs[i].configSection, inputs[i].configKey, buffer, iniPath);
         return;
      }
   }
}

void AddUniqueToListBox(HWND hwndOwner, HWND hListBox, LPCWSTR itemText)
{
   DEBUG_PRINT(itemText);
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
         DWORD attr = GetFileAttributesW(filePath);
         if (attr != INVALID_FILE_ATTRIBUTES)
         {
            // Optionally prefix folders to distinguish them later - Not in use right now
            if (attr & FILE_ATTRIBUTE_DIRECTORY)
            {
               // You can prefix with "FOLDER:" or similar
               wchar_t folderEntry[MAX_PATH + 10];
               // swprintf(folderEntry, MAX_PATH + 10, L"\xD83D\xDCC1 %s", filePath);
               swprintf(folderEntry, MAX_PATH + 10, L"%s", filePath);
               AddUniqueToListBox(hwnd, hListBox, folderEntry);
            }
            else
            {
               AddUniqueToListBox(hwnd, hListBox, filePath);
            }
         }
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

void update_output_type_dropdown()
{
   BOOL hasWinRAR = is_valid_winrar(3); // Mode 3 = compression
   BOOL hasMuPDF = is_valid_mutool();

   SendMessageW(hOutputType, CB_RESETCONTENT, 0, 0);
   SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"CBZ");

   if (hasWinRAR)
   {
      SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"Keep original");
      SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"CBR");
   }

   if (hasMuPDF)
   {
      SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"PDF");
   }

   // Default selection
   LRESULT index = SendMessageW(hOutputType, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)L"Keep original");
   SendMessageW(hOutputType, CB_SETCURSEL, (WPARAM)(index != CB_ERR ? index : 0), 0);
}

void load_config_values(void)
{
   wchar_t buffer[MAX_PATH];

   // Build config.ini path
   wchar_t iniPath[MAX_PATH];
   GetModuleFileNameW(NULL, iniPath, MAX_PATH);
   wchar_t *lastSlash = wcsrchr(iniPath, L'\\');
   if (lastSlash)
      *(lastSlash + 1) = L'\0';
   wcscat(iniPath, L"config.ini");

   typedef struct
   {
      const wchar_t *section;
      const wchar_t *key;
      HWND *hwnd;
      wchar_t *target;
      DWORD size;
   } ConfigBinding;

   ConfigBinding bindings[] = {
       {L"Paths", L"TMP_FOLDER", &hTmpFolder, g_config.TMP_FOLDER, MAX_PATH},
       {L"Paths", L"OUTPUT_FOLDER", &hOutputFolder, g_config.OUTPUT_FOLDER, MAX_PATH},
       {L"Paths", L"WINRAR_PATH", &hWinrarPath, g_config.WINRAR_PATH, MAX_PATH},
       {L"Paths", L"SEVEN_ZIP_PATH", &hSevenZipPath, g_config.SEVEN_ZIP_PATH, MAX_PATH},
       {L"Paths", L"IMAGEMAGICK_PATH", &hImageMagickPath, g_config.IMAGEMAGICK_PATH, MAX_PATH},
       {L"Paths", L"MUTOOL_PATH", &hMuToolPath, g_config.MUTOOL_PATH, MAX_PATH},
       {L"Image", L"IMAGE_SIZE_WIDTH", &hImageSizeWidth, g_config.IMAGE_SIZE_WIDTH, IMG_DIM_LEN},
       {L"Image", L"IMAGE_SIZE_HEIGHT", &hImageSizeHeight, g_config.IMAGE_SIZE_HEIGHT, IMG_DIM_LEN},
       {L"Image", L"IMAGE_QUALITY", &hImageQualityValue, g_config.IMAGE_QUALITY, QUALITY_LEN},
       {L"Image", L"IMAGE_TYPE", &hImageType, g_config.IMAGE_TYPE, IMAGE_TYPE_LEN}};

   for (size_t i = 0; i < sizeof(bindings) / sizeof(bindings[0]); ++i)
   {
      const ConfigBinding *b = &bindings[i];
      GetPrivateProfileStringW(b->section, b->key, L"", buffer, MAX_PATH, iniPath);

      if (b->target)
      {
         wcsncpy(b->target, buffer, b->size - 1);
         b->target[b->size - 1] = L'\0';
      }

      if (b->hwnd && *b->hwnd)
      {
         // IMAGE_TYPE uses dropdown, match by label
         if (*b->hwnd == hImageType)
         {
            for (int j = 0; j < IMAGE_TYPE_COUNT; ++j)
            {
               if (wcscmp(buffer, g_ImageTypeOptions[j].label) == 0)
               {
                  SendMessageW(hImageType, CB_SETCURSEL, (WPARAM)j, 0);
                  break;
               }
            }
         }
         else
         {
            SetWindowTextW(*b->hwnd, buffer);
         }
      }

      if (wcscmp(b->key, L"IMAGE_QUALITY") == 0)
      {
         SendMessageW(hImageQualitySlider, TBM_SETPOS, TRUE, _wtoi(buffer));
      }
   }

   for (int i = 0; i < controlCount; ++i)
   {
      GetPrivateProfileStringW(controls[i].configSegment, controls[i].configKey, L"false", buffer, sizeof(buffer), iniPath);

      BOOL isChecked = (wcscmp(buffer, L"true") == 0);

      SendMessageW(*controls[i].hCheckbox, BM_SETCHECK, isChecked ? BST_CHECKED : BST_UNCHECKED, 0);

      if (controls[i].configField)
         *controls[i].configField = isChecked;
   }

   update_output_type_dropdown();
}

static HHOOK hHook;
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
   if (nCode == HCBT_ACTIVATE)
   {
      HWND hMsgBox = (HWND)wParam;

      RECT rcOwner, rcMsgBox;
      GetWindowRect(GetParent(hMsgBox), &rcOwner);
      GetWindowRect(hMsgBox, &rcMsgBox);

      int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - (rcMsgBox.right - rcMsgBox.left)) / 2;
      int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - (rcMsgBox.bottom - rcMsgBox.top)) / 2;

      SetWindowPos(hMsgBox, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
      UnhookWindowsHookEx(hHook);
   }

   return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int MessageBoxCentered(HWND hwnd, LPCWSTR text, LPCWSTR caption, UINT type)
{
   hHook = SetWindowsHookEx(WH_CBT, CBTProc, NULL, GetCurrentThreadId());
   return MessageBoxW(hwnd, text, caption, type);
}