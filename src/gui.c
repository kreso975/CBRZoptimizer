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
    {L"Temp Folder:", L"TMP_FOLDER", L"Paths", g_config.TMP_FOLDER, 30, 100, 200, &hTmpFolderLabel, &hTmpFolder, L"Select temporary folder for processing"},
    {L"Output Folder:", L"OUTPUT_FOLDER", L"Paths", g_config.OUTPUT_FOLDER, 60, 100, 200, &hOutputFolderLabel, &hOutputFolder, L"Select Output folder for copying"},
    {L"WinRAR Path:", L"WINRAR_PATH", L"Paths", g_config.WINRAR_PATH, 90, 100, 200, &hWinrarLabel, &hWinrarPath, L"Select WinRar.exe"},
    {L"7-Zip Path:", L"SEVEN_ZIP_PATH", L"Paths", g_config.SEVEN_ZIP_PATH, 120, 100, 200, &hSevenZipLabel, &hSevenZipPath, L"Select 7z.exe"},
    {L"ImageMagick:", L"IMAGEMAGICK_PATH", L"Paths", g_config.IMAGEMAGICK_PATH, 150, 100, 200, &hImageMagickLabel, &hImageMagickPath, L"Select ImageMagick.exe"},
    {L"MuPDF Tool:", L"MUTOOL_PATH", L"Paths", g_config.MUTOOL_PATH, 180, 100, 200, &hMuToolLabel, &hMuToolPath, L"Select mutool.exe"}};
const size_t inputsCount = sizeof(inputs) / sizeof(inputs[0]);

LabelCheckboxPair controls[] = {
    {L"Image optimization", L"OUTPUT_RUN_IMAGE_OPTIMIZER", L"Output", 450, &hOutputRunImageOptimizer, &hOutputRunImageOptimizerLabel, &g_config.runImageOptimizer, TRUE},
    {L"Compress folder", L"OUTPUT_RUN_COMPRESSOR", L"Output", 470, &hOutputRunCompressor, &hOutputRunCompressorLabel, &g_config.runCompressor, TRUE},
    {L"Keep extracted folders", L"OUTPUT_KEEP_EXTRACTED", L"Output", 490, &hOutputKeepExtracted, &hOutputKeepExtractedLabel, &g_config.keepExtracted, FALSE},
    {L"Extract cover image", L"OUTPUT_EXTRACT_COVER", L"Output", 510, &hOutputExtractCover, &hOutputExtractCoverLabel, &g_config.extractCover, TRUE},
    {L"Resize image:", L"IMAGE_RESIZE_TO", L"Image", 490, &hImageResizeTo, &hImageResizeToLabel, &g_config.resizeTo, TRUE},
    {L"Keep Aspect Ratio", L"IMAGE_KEEP_ASPECT_RATIO", L"Image", 490, &hImageKeepAspectRatio, &hImageKeepAspectRatioLabel, &g_config.keepAspectRatio, TRUE},
    {L"Allow upscaling", L"IMAGE_ALLOW_UPSCALING", L"Image", 490, &hImageAllowUpscaling, &hImageAllowUpscalingLabel, &g_config.allowUpscaling, FALSE}};

const size_t controlCount = sizeof(controls) / sizeof(controls[0]);

ImageFieldBinding imageFields[] = {
    {L"IMAGE_SIZE_WIDTH", g_config.IMAGE_SIZE_WIDTH, IMG_DIM_LEN, &hImageSizeWidth, FALSE, FALSE},
    {L"IMAGE_SIZE_HEIGHT", g_config.IMAGE_SIZE_HEIGHT, IMG_DIM_LEN, &hImageSizeHeight, FALSE, FALSE},
    {L"IMAGE_QUALITY", g_config.IMAGE_QUALITY, QUALITY_LEN, &hImageQualityValue, TRUE, FALSE},
    {L"IMAGE_TYPE", g_config.IMAGE_TYPE, IMAGE_TYPE_LEN, &hImageType, FALSE, TRUE}};

const size_t imageFieldsCount = sizeof(imageFields) / sizeof(imageFields[0]);

ButtonSpec buttons[] = {
    // Existing buttons...
    {&hRemoveButton, 20, 30, ID_REMOVE_BUTTON, IDB_BUTTON_MINUS, L"Remove selected item", &hButtonMinus, 32, 32, FALSE, TRUE},
    {&hAddButton, 70, 30, ID_ADD_BUTTON, IDB_BUTTON_PLUS, L"Add file to list", &hButtonPlus, 32, 32, FALSE, TRUE},
    {&hAddFolderButton, 120, 30, ID_ADD_FOLDER_BUTTON, IDB_BUTTON_ADD_FOLDER, L"Add folder to list", &hButtonAddFolder, 32, 32, FALSE, TRUE},
    {&hOpenInTmpFolderButton, 170, 30, ID_OPEN_IN_TMP_FOLDER_BUTTON, IDB_OPEN_IN_FOLDER, L"Open TMP Folder", &hButtonOpenInFolder, 16, 16, FALSE, TRUE},
    {&hOpenInOutputFolderButton, 170, 30, ID_OPEN_IN_OUTPUT_FOLDER_BUTTON, IDB_OPEN_IN_FOLDER, L"Open Output Folder", &hButtonOpenInFolder, 16, 16, FALSE, TRUE},
    {&hStartButton, 20, 230, ID_START_BUTTON, IDB_BUTTON_START, L"Start process", &hButtonStart, 70, 30, FALSE, TRUE},
    {&hStopButton, 100, 230, ID_STOP_BUTTON, IDB_BUTTON_STOP, L"Stop process", &hButtonStop, 70, 30, FALSE, FALSE},

    // Browse buttons
    {&hTmpBrowse, 310, 30, ID_TMP_FOLDER_BROWSE, IDB_BUTTON_ADD, L"Select temporary folder for processing", &hButtonBrowse, 26, 26, FALSE, TRUE},
    {&hOutputBrowse, 310, 60, ID_OUTPUT_FOLDER_BROWSE, IDB_BUTTON_ADD, L"Select Output folder for copying", &hButtonBrowse, 26, 26, FALSE, TRUE},
    {&hWinrarBrowse, 310, 90, ID_WINRAR_PATH_BROWSE, IDB_BUTTON_ADD, L"Select WinRar.exe", &hButtonBrowse, 26, 26, FALSE, TRUE},
    {&hSevenZipBrowse, 310, 120, ID_SEVEN_ZIP_PATH_BROWSE, IDB_BUTTON_ADD, L"Select 7z.exe", &hButtonBrowse, 26, 26, FALSE, TRUE},
    {&hImageMagickBrowse, 310, 150, ID_IMAGEMAGICK_PATH_BROWSE, IDB_BUTTON_ADD, L"Select ImageMagick.exe", &hButtonBrowse, 26, 26, FALSE, TRUE},
    {&hMuToolBrowse, 310, 180, ID_MUTOOL_PATH_BROWSE, IDB_BUTTON_ADD, L"Select mutool.exe", &hButtonBrowse, 26, 26, FALSE, TRUE}
};

const size_t buttonsCount = sizeof(buttons) / sizeof(buttons[0]);

void SendStatus(HWND hwnd, UINT messageId, const wchar_t *prefix, const wchar_t *info)
{
   wchar_t buffer[512];
   swprintf(buffer, 512, L"%s%s", prefix, info);
   PostMessageW(hwnd, messageId, 0, (LPARAM)_wcsdup(buffer));
}

// Enables or disables a variable number of HWND controls.
// Usage: SetControlsEnabled(TRUE, 3, hwnd1, hwnd2, hwnd3);
// Each HWND passed will be enabled or disabled based on the 'enable' flag.
void SetControlsEnabled(BOOL enable, int count, ...)
{
   va_list args;
   va_start(args, count);

   for (int i = 0; i < count; ++i)
   {
      HWND h = va_arg(args, HWND);
      if (h)
         EnableWindow(h, enable);
   }

   va_end(args);
}

// EnableResizeGroupWithLogic(L"FilesGroup", FALSE);
// EnableResizeGroupWithLogic(L"FilesGroup", TRUE);
void EnableResizeGroupWithLogic(LPCWSTR groupName, BOOL enable)
{
   DEBUG_PRINTF(L"[DEBUG] EnableResizeGroupWithLogic entered: group = \"%s\", enable = %s\n",
                groupName, enable ? L"TRUE" : L"FALSE");

   // 1. Handle "Resize To" checkbox and label independently
   BOOL forceEnableResizeToggle = enable && g_config.runImageOptimizer;
   SetControlsEnabled(forceEnableResizeToggle, 2, hImageResizeToLabel, hImageResizeTo);
   DEBUG_PRINTF(L"[DEBUG] Resize toggle = %s\n", forceEnableResizeToggle ? L"TRUE" : L"FALSE");

   // 2–3. Toggle group controls based on 'enable' flag (excluding "Resize To")
   for (int i = 0; i < groupElementsCount; ++i)
   {
      if (wcscmp(groupElements[i].group, groupName) != 0 || !*groupElements[i].hwndPtr)
         continue;

      HWND h = *groupElements[i].hwndPtr;

      // Skip ResizeTo controls
      if (h == hImageResizeTo || h == hImageResizeToLabel)
         continue;

      EnableWindow(h, enable);
   }

   // ⚠️ Do not exit early — continue to refresh all dependent UI

   // 4. Toggle advanced controls based on resize settings
   if (g_config.runImageOptimizer)
   {
      DEBUG_PRINT(L"[DEBUG] runImageOptimizer = TRUE → processing image UI");
      SetControlsEnabled(g_config.resizeTo, 4,
                         hImageKeepAspectRatioLabel, hImageKeepAspectRatio,
                         hImageAllowUpscalingLabel, hImageAllowUpscaling);

      DEBUG_PRINT(g_config.resizeTo ? L"[DEBUG] ResizeTo = TRUE" : L"[DEBUG] ResizeTo = FALSE");

      if (!g_config.resizeTo)
      {
         DEBUG_PRINT(L"[DEBUG] ResizeTo is OFF → disabling dimensions");
         SetControlsEnabled(FALSE, 4,
                            hImageSizeWidthLabel, hImageSizeWidth,
                            hImageSizeHeightLabel, hImageSizeHeight);
      }
      else if (!g_config.keepAspectRatio)
      {
         DEBUG_PRINT(L"[DEBUG] KeepAspectRatio = FALSE → enabling both dimensions");
         SetControlsEnabled(TRUE, 4,
                            hImageSizeWidthLabel, hImageSizeWidth,
                            hImageSizeHeightLabel, hImageSizeHeight);
      }
      else
      {
         if (wcscmp(g_config.IMAGE_TYPE, L"Portrait") == 0)
         {
            DEBUG_PRINT(L"[DEBUG] Portrait mode");
            SetControlsEnabled(FALSE, 2, hImageSizeWidthLabel, hImageSizeWidth);
            SetControlsEnabled(TRUE, 2, hImageSizeHeightLabel, hImageSizeHeight);
         }
         else if (wcscmp(g_config.IMAGE_TYPE, L"Landscape") == 0)
         {
            DEBUG_PRINT(L"[DEBUG] Landscape mode");
            SetControlsEnabled(TRUE, 2, hImageSizeWidthLabel, hImageSizeWidth);
            SetControlsEnabled(FALSE, 2, hImageSizeHeightLabel, hImageSizeHeight);
         }
#ifdef _DEBUG
         else
         {
            DEBUG_PRINTF(L"[DEBUG] Unexpected IMAGE_TYPE: %s\n", g_config.IMAGE_TYPE);
            SetControlsEnabled(FALSE, 4,
                               hImageSizeWidthLabel, hImageSizeWidth,
                               hImageSizeHeightLabel, hImageSizeHeight);
         }
#endif
      }
   }
   else
   {
      DEBUG_PRINT(L"[DEBUG] runImageOptimizer = FALSE → disabling all image resize controls");
      SetControlsEnabled(FALSE, 10,
                         hImageResizeToLabel, hImageResizeTo,
                         hImageKeepAspectRatioLabel, hImageKeepAspectRatio,
                         hImageAllowUpscalingLabel, hImageAllowUpscaling,
                         hImageSizeWidthLabel, hImageSizeWidth,
                         hImageSizeHeightLabel, hImageSizeHeight);
   }

   // Compressor logic is always refreshed
   DEBUG_PRINT(g_config.extractCover
                   ? L"[DEBUG] ExtractCover = TRUE → disabling compression controls"
                   : L"[DEBUG] ExtractCover = FALSE → compression controls depend on RunCompressor");

   SetControlsEnabled(!g_config.extractCover, 2,
                      hOutputRunCompressor, hOutputRunCompressorLabel);

   if (!g_config.extractCover && g_config.runCompressor)
   {
      DEBUG_PRINT(L"[DEBUG] Format selection ENABLED");
      SetControlsEnabled(TRUE, 2, hOutputType, hOutputTypeLabel);
   }
   else
   {
      DEBUG_PRINT(L"[DEBUG] Format selection DISABLED");
      SetControlsEnabled(FALSE, 2, hOutputType, hOutputTypeLabel);
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
         SendMessageW(hListBox, LB_DELETESTRING, (WPARAM)(INT_PTR)selectedIndexes[i], 0);

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
      SendMessageW(hOutputType, CB_ADDSTRING, 0, (LPARAM)L"PDF");

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

    // 1. Load "Paths" section using inputs[]
    for (size_t i = 0; i < ARRAYSIZE(inputs); ++i)
    {
        GetPrivateProfileStringW(inputs[i].configSection, inputs[i].configKey, L"", buffer, MAX_PATH, iniPath);

        // Copy to config
        wcsncpy(inputs[i].defaultText, buffer, MAX_PATH - 1);
        inputs[i].defaultText[MAX_PATH - 1] = L'\0';

        // Update UI
        if (inputs[i].hEdit)
            SetWindowTextW(*inputs[i].hEdit, buffer);
    }

    // 2. Fallback: TMP_FOLDER → System temp with subfolder if empty
    if (wcslen(g_config.TMP_FOLDER) == 0)
    {
        wchar_t defaultTmp[MAX_PATH];
        DWORD len = GetTempPathW(MAX_PATH, defaultTmp);

        if (len > 0 && len < MAX_PATH)
        {
            PathAppendW(defaultTmp, L"CBRZoptimizer");
            CreateDirectoryW(defaultTmp, NULL); // Safe if exists
            wcsncpy_s(g_config.TMP_FOLDER, MAX_PATH, defaultTmp, _TRUNCATE);
        }
        else
        {
            wcsncpy_s(g_config.TMP_FOLDER, MAX_PATH, L"C:\\Temp", _TRUNCATE);
            CreateDirectoryW(g_config.TMP_FOLDER, NULL);
        }

        // Update TMP_FOLDER UI field
        if (hTmpFolder)
            SetWindowTextW(hTmpFolder, g_config.TMP_FOLDER);
    }

    // 3. Fallback: OUTPUT_FOLDER → same as TMP_FOLDER if empty
    if (wcslen(g_config.OUTPUT_FOLDER) == 0 && wcslen(g_config.TMP_FOLDER) > 0)
    {
        wcsncpy_s(g_config.OUTPUT_FOLDER, MAX_PATH, g_config.TMP_FOLDER, _TRUNCATE);

        // Update OUTPUT_FOLDER UI field
        if (hOutputFolder)
            SetWindowTextW(hOutputFolder, g_config.OUTPUT_FOLDER);
    }

    // 4. Load "Image" section and update controls
    for (size_t i = 0; i < ARRAYSIZE(imageFields); ++i)
    {
        GetPrivateProfileStringW(L"Image", imageFields[i].key, L"", buffer, imageFields[i].size, iniPath);
        wcsncpy_s(imageFields[i].target, imageFields[i].size, buffer, _TRUNCATE);

        if (imageFields[i].hwnd)
        {
            if (imageFields[i].isDropdown)
            {
                for (int j = 0; j < IMAGE_TYPE_COUNT; ++j)
                {
                    if (wcscmp(buffer, g_ImageTypeOptions[j].label) == 0)
                    {
                        SendMessageW(*imageFields[i].hwnd, CB_SETCURSEL, (WPARAM)j, 0);
                        break;
                    }
                }
            }
            else
            {
                SetWindowTextW(*imageFields[i].hwnd, buffer);

                if (imageFields[i].isSlider)
                    SendMessageW(hImageQualitySlider, TBM_SETPOS, TRUE, _wtoi(buffer));
            }
        }
    }

    // 5. Load checkbox states from controls[]
    for (size_t i = 0; i < controlCount; ++i)
    {
        GetPrivateProfileStringW(controls[i].configSegment, controls[i].configKey, L"false", buffer, MAX_PATH, iniPath);

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