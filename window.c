#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>
#include <stdbool.h>
#include "resource.h"
#include "gui.h"
#include "functions.h"
#include "aboutDialog.h"

#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

volatile BOOL g_StopProcessing = FALSE;
HINSTANCE g_hInstance;

HFONT hBoldFont, hFontLabel, hFontInput, hFontEmoji;

WNDPROC gOriginalLabelProc = NULL;

HBITMAP hButtonPlus, hButtonMinus, hButtonBrowse, hButtonStart;

HWND hListBox, hStartButton, hStopButton, hAddButton, hRemoveButton, hSettingsWnd;
HWND hTmpFolder, hOutputFolder, hWinrarPath, hSevenZipPath, hImageMagickPath, hImageResize;
HWND hTmpBrowse, hOutputBrowse, hWinrarBrowse, hSevenZipBrowse, hImageMagickBrowse;
HWND hFilesGroup, hTerminalGroup, hSettingsGroup, hImageSettingsGroup, hOutputGroup;
HWND hTerminalProcessingLabel, hTerminalProcessingText, hTerminalText;
HWND hTmpFolderLabel, hOutputFolderLabel, hWinrarLabel, hSevenZipLabel, hImageMagickLabel, hImageQualityLabel, hImageQualityValue, hImageDpiLabel, hImageSizeLabel;
HWND hImageQualitySlider, hImageDpi, hImageSize;
HWND hOutputKeepExtractedLabel, hOutputKeepExtracted, hOutputRunExtractLabel, hOutputRunExtract;
HWND hOutputType, hOutputTypeLabel, hOutputRunImageOptimizer, hOutputRunCompressor, hOutputRunImageOptimizerLabel, hOutputRunCompressorLabel;
HWND hMenuBar, hFileMenu, hHelpMenu; // **Added Menu Handles**

wchar_t TMP_FOLDER[MAX_PATH];
wchar_t OUTPUT_FOLDER[MAX_PATH];
wchar_t WINRAR_PATH[MAX_PATH];
wchar_t IMAGEMAGICK_PATH[MAX_PATH];
wchar_t SEVEN_ZIP_PATH[MAX_PATH];
wchar_t IMAGE_SIZE[16];
wchar_t IMAGE_QUALITY[8];
wchar_t IMAGE_DPI[8];

LabelCheckboxPair controls[] = {
    {L"Image optimization", L"hOutputRunImageOptimizer", 450, &hOutputRunImageOptimizer, &hOutputRunImageOptimizerLabel},
    {L"Compress folder", L"hOutputRunCompressor", 470, &hOutputRunCompressor, &hOutputRunCompressorLabel},
    {L"Keep extracted folders", L"hOutputKeepExtracted", 490, &hOutputKeepExtracted, &hOutputKeepExtractedLabel}};

const int controlCount = sizeof(controls) / sizeof(controls[0]);

// Adjust UI elements dynamically when resizing
void AdjustLayout(HWND hwnd)
{
   RECT rcClient;
   GetClientRect(hwnd, &rcClient);

   // Resize ListBox to fit window
   SetWindowPos(hListBox, NULL, 10, 10, rcClient.right - 20, rcClient.bottom - 200, SWP_NOZORDER);

   // Keep buttons at the bottom
   SetWindowPos(hStartButton, NULL, 120, rcClient.bottom - 40, 100, 30, SWP_NOZORDER);
}

WNDPROC oldListBoxProc; // Global variable to store the original ListBox procedure
// Custom List Box Procedure
LRESULT CALLBACK ListBoxProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_KEYDOWN:
      if (wParam == VK_DELETE || wParam == VK_BACK) // Check for Delete or Backspace
      {
         RemoveSelectedItems(hListBox);
      }
      break;
   }

   return CallWindowProc(oldListBoxProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK LabelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   if (msg == WM_LBUTTONDOWN)
   {
      HWND hCheckbox = (HWND)GetProp(hwnd, L"CheckboxHandle");
      if (hCheckbox)
      {
         LRESULT state = SendMessage(hCheckbox, BM_GETCHECK, 0, 0);
         LRESULT newState = (state == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED;
         SendMessage(hCheckbox, BM_SETCHECK, newState, 0);

         // Immediately save to config.ini here
         wchar_t iniPath[MAX_PATH];
         GetModuleFileNameW(NULL, iniPath, MAX_PATH);
         wchar_t *lastSlash = wcsrchr(iniPath, L'\\');
         if (lastSlash)
            *(lastSlash + 1) = '\0';
         wcscat(iniPath, L"config.ini");

         extern LabelCheckboxPair controls[]; // Make sure LabelProc sees the array
         extern const int controlCount;

         for (int i = 0; i < controlCount; ++i)
         {
            if (hCheckbox == *controls[i].hCheckbox)
            {
               WritePrivateProfileStringW(L"Output", controls[i].configKey, (newState == BST_CHECKED) ? L"true" : L"false", iniPath);
               break;
            }
         }
      }
   }

   // Fall through to original label drawing behavior
   WNDPROC orig = (WNDPROC)GetProp(hwnd, L"OrigProc");
   return CallWindowProc(orig ? orig : DefWindowProc, hwnd, msg, wParam, lParam);
}

BOOL isHoverRemove = FALSE;
BOOL isHoverAdd = FALSE;
BOOL isHoverStart = FALSE;
BOOL isHoverTmp = FALSE;
BOOL isHoverOutput = FALSE;
BOOL isHoverWinrar = FALSE;
BOOL isHoverSevenZip = FALSE;
BOOL isHoverImageMagick = FALSE;

LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static TRACKMOUSEEVENT tme;
   int ctlId = GetDlgCtrlID(hwnd);
   BOOL handled = FALSE;
   BOOL *hoverFlag = NULL;

   switch (ctlId)
   {
   case ID_REMOVE_BUTTON:
      hoverFlag = &isHoverRemove;
      break;
   case ID_ADD_BUTTON:
      hoverFlag = &isHoverAdd;
      break;
   case ID_START_BUTTON:
      hoverFlag = &isHoverStart;
      break;
   case ID_TMP_FOLDER_BROWSE:
      hoverFlag = &isHoverTmp;
      break;
   case ID_OUTPUT_FOLDER_BROWSE:
      hoverFlag = &isHoverOutput;
      break;
   case ID_WINRAR_PATH_BROWSE:
      hoverFlag = &isHoverWinrar;
      break;
   case ID_SEVEN_ZIP_PATH_BROWSE:
      hoverFlag = &isHoverSevenZip;
      break;
   case ID_IMAGEMAGICK_PATH_BROWSE:
      hoverFlag = &isHoverImageMagick;
      break;
   }

   switch (msg)
   {
   case WM_MOUSEMOVE:
      if (hoverFlag && !*hoverFlag)
      {
         *hoverFlag = TRUE;
         tme = (TRACKMOUSEEVENT){sizeof(tme), TME_LEAVE, hwnd, 0};
         TrackMouseEvent(&tme);
         InvalidateRect(hwnd, NULL, TRUE);
      }
      break;

   case WM_MOUSELEAVE:
      if (hoverFlag && *hoverFlag)
      {
         *hoverFlag = FALSE;
         InvalidateRect(hwnd, NULL, TRUE);
      }
      break;

   default:
      return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, msg, wParam, lParam);
   }

   return 0;
}

BOOL isBrowseButtonHovered(UINT ctlId)
{
   switch (ctlId)
   {
   case ID_TMP_FOLDER_BROWSE:
      return isHoverTmp;
   case ID_OUTPUT_FOLDER_BROWSE:
      return isHoverOutput;
   case ID_WINRAR_PATH_BROWSE:
      return isHoverWinrar;
   case ID_SEVEN_ZIP_PATH_BROWSE:
      return isHoverSevenZip;
   case ID_IMAGEMAGICK_PATH_BROWSE:
      return isHoverImageMagick;
   default:
      return FALSE;
   }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   PAINTSTRUCT ps;
   HDC hdc;
   RECT rect;

   switch (msg)
   {
   case WM_CREATE:
      LOGFONT lf = {0};
      GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
      lf.lfHeight = -16;     // Negative = height in pixels; positive = point size (sort of)
      lf.lfWeight = FW_BOLD; // FW_NORMAL for regular weight
      // lf.lfItalic = TRUE; // uncomment for italic

      hBoldFont = CreateFontIndirect(&lf);

      hFontInput = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

      hFontLabel = CreateFontW(-13, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

      hFontEmoji = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Emoji");

      HBRUSH hGrayBrush;
      hGrayBrush = CreateSolidBrush(RGB(192, 192, 192));

      int defaultWidth = 960;  // 800 * 1.2 (20% increase)
      int defaultHeight = 600; // Keep height unchanged unless needed
      MoveWindow(hwnd, 100, 100, defaultWidth, defaultHeight, TRUE);

      HMENU hMenu = CreateMenu();
      HMENU hFileMenu = CreatePopupMenu();
      HMENU hHelpMenu = CreatePopupMenu();

      AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"Exit");
      AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_ABOUT, L"About");

      AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
      AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Help");

      // Correct way to right-align "Help"
      MENUITEMINFO mii = {0};
      mii.cbSize = sizeof(MENUITEMINFO);
      mii.fMask = MIIM_FTYPE;
      mii.fType = MFT_RIGHTJUSTIFY;

      SetMenuItemInfo(hMenu, GetMenuItemCount(hMenu) - 1, TRUE, &mii);

      SetMenu(hwnd, hMenu);

      // **Files Group (Left)**
      hFilesGroup = CreateWindowW(L"BUTTON", L"Files", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                  10, 10, 300, 200, hwnd, NULL, NULL, NULL);

      // Create static controls for images (borderless)
      hRemoveButton = CreateWindowW(
          L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
          20, 30, 32, 32, hwnd, (HMENU)ID_REMOVE_BUTTON, NULL, NULL);

      hAddButton = CreateWindowW(
          L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
          70, 30, 32, 32, hwnd, (HMENU)ID_ADD_BUTTON, NULL, NULL);

      // Load BMP images
      hButtonPlus = LoadBMP(L"icons/plus-32b.bmp");
      hButtonMinus = LoadBMP(L"icons/minus-32b.bmp");

      if (!hButtonPlus)
         MessageBoxW(hwnd, L"Failed to load plus image!", L"Error", MB_OK | MB_ICONERROR);
      else
         SendMessageW(hAddButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hButtonPlus);

      if (!hButtonMinus)
         MessageBoxW(hwnd, L"Failed to load minus image!", L"Error", MB_OK | MB_ICONERROR);
      else
         SendMessageW(hRemoveButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hButtonMinus);

      SetWindowLongPtr(hRemoveButton, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(hRemoveButton, GWLP_WNDPROC));
      SetWindowLongPtr(hRemoveButton, GWLP_WNDPROC, (LONG_PTR)ButtonProc);

      SetWindowLongPtr(hAddButton, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(hAddButton, GWLP_WNDPROC));
      SetWindowLongPtr(hAddButton, GWLP_WNDPROC, (LONG_PTR)ButtonProc);

      // **Listbox settings**
      hListBox = CreateWindowW(L"LISTBOX", NULL,
                               WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL | LBS_NOTIFY,
                               20, 70, 270, 150, hwnd, (HMENU)ID_LISTBOX, NULL, NULL);
      if (hListBox)
         SendMessageW(hListBox, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      oldListBoxProc = (WNDPROC)SetWindowLongPtr(hListBox, GWLP_WNDPROC, (LONG_PTR)ListBoxProc); // Subclass ListBox
      DragAcceptFiles(hwnd, TRUE);
      SetFocus(hListBox); // Ensure ListBox gets keyboard input

      // **Terminal Group (Left)**
      hTerminalGroup = CreateWindowW(L"BUTTON", L"Terminal", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                     10, 10, 300, 40, hwnd, NULL, NULL, NULL);
      hTerminalProcessingLabel = CreateWindowW(L"STATIC", L"Processing:", WS_CHILD, 430, 210, 100, 20, hwnd, NULL, NULL, NULL);
      hTerminalProcessingText = CreateWindowW(L"STATIC", L"", WS_CHILD, 430, 210, 50, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hTerminalProcessingLabel, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
      SendMessageW(hTerminalProcessingText, WM_SETFONT, (WPARAM)hBoldFont, TRUE);

      hTerminalText = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 430, 210, 50, 20, hwnd, NULL, NULL, NULL);
      if (hTerminalText)
         SendMessageW(hListBox, WM_SETFONT, (WPARAM)hFontEmoji, TRUE);

      hStartButton = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                   20, 230, 120, 30, hwnd, (HMENU)ID_START_BUTTON, NULL, NULL);

      hButtonStart = LoadBMP(L"icons/go_logo_icon_30.bmp");
      if (!hButtonStart)
         MessageBoxW(hwnd, L"Failed to load start image!", L"Error", MB_OK | MB_ICONERROR);
      else
         SendMessageW(hStartButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hButtonStart);

      SetWindowLongPtr(hStartButton, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(hStartButton, GWLP_WNDPROC));
      SetWindowLongPtr(hStartButton, GWLP_WNDPROC, (LONG_PTR)ButtonProc);

      hStopButton = CreateWindowW(L"BUTTON", L"Stop", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  20, 230, 60, 30, hwnd, (HMENU)ID_STOP_BUTTON, NULL, NULL);

      // **Settings Group (Right)**
      hSettingsGroup = CreateWindowW(L"BUTTON", L"Settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                     320, 10, 480, 210, hwnd, NULL, NULL, NULL);

      hButtonBrowse = LoadBMP(L"icons/new_add_insert_file_32.bmp");

      typedef struct
      {
         const wchar_t *labelText; // Label displayed next to the field
         wchar_t *defaultText;     // Default content (e.g. TMP_FOLDER)
         int y;                    // Vertical position
         int labelWidth;           // Width of the label
         int editWidth;            // Width of the edit box
         int browseId;             // Command ID for the Browse button
         HWND *hLabel;             // Pointer to the label control handle
         HWND *hEdit;              // Pointer to the edit box handle
         HWND *hBrowse;            // Pointer to the browse button handle
         HBITMAP *hBitmap;         // Pointer to optional image (e.g. folder icon)
         const wchar_t *bmpPath;   // Path to bitmap image to load
      } EditBrowseControl;

      EditBrowseControl inputs[] = {
          {L"Temp Folder:", TMP_FOLDER, 30, 100, 200, ID_TMP_FOLDER_BROWSE, &hTmpFolderLabel, &hTmpFolder, &hTmpBrowse, &hButtonBrowse, L"icons/new_add_insert_file_32.bmp"},
          {L"Output Folder:", OUTPUT_FOLDER, 60, 100, 200, ID_OUTPUT_FOLDER_BROWSE, &hOutputFolderLabel, &hOutputFolder, &hOutputBrowse, NULL, NULL},
          {L"WinRAR Path:", WINRAR_PATH, 90, 100, 200, ID_WINRAR_PATH_BROWSE, &hWinrarLabel, &hWinrarPath, &hWinrarBrowse, NULL, NULL},
          {L"7-Zip Path:", SEVEN_ZIP_PATH, 120, 100, 200, ID_SEVEN_ZIP_PATH_BROWSE, &hSevenZipLabel, &hSevenZipPath, &hSevenZipBrowse, NULL, NULL},
          {L"ImageMagick:", IMAGEMAGICK_PATH, 150, 100, 200, ID_IMAGEMAGICK_PATH_BROWSE, &hImageMagickLabel, &hImageMagickPath, &hImageMagickBrowse, NULL, NULL}};

      for (int i = 0; i < ARRAYSIZE(inputs); ++i)
      {
         *inputs[i].hLabel = CreateWindowW(L"STATIC", inputs[i].labelText, WS_CHILD | WS_VISIBLE, 20, inputs[i].y, inputs[i].labelWidth, 20, hwnd, NULL, NULL, NULL);
         *inputs[i].hEdit = CreateWindowW(L"EDIT", inputs[i].defaultText, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 120, inputs[i].y, inputs[i].editWidth, 20, hwnd, NULL, NULL, NULL);
         *inputs[i].hBrowse = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 320, inputs[i].y, 32, 32, hwnd, (HMENU)(UINT_PTR)inputs[i].browseId, NULL, NULL);

         if (inputs[i].bmpPath && inputs[i].hBitmap)
         {
            *inputs[i].hBitmap = (HBITMAP)LoadImageW(NULL, inputs[i].bmpPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
         }

         if (*inputs[i].hBrowse)
         {
            SendMessageW(*inputs[i].hLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
            SendMessageW(*inputs[i].hEdit, WM_SETFONT, (WPARAM)hFontInput, TRUE);
            SetWindowLongPtr(*inputs[i].hBrowse, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(*inputs[i].hBrowse, GWLP_WNDPROC));
            SetWindowLongPtr(*inputs[i].hBrowse, GWLP_WNDPROC, (LONG_PTR)ButtonProc);
         }
      }

      // **Image Settings Group (Right)**
      hImageSettingsGroup = CreateWindowW(L"BUTTON", L"Image Settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                          320, 190, 480, 120, hwnd, NULL, NULL, NULL);

      hImageQualityLabel = CreateWindowW(L"STATIC", L"Image Quality:", WS_CHILD | WS_VISIBLE, 330, 210, 120, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hImageQualityLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
      hImageQualityValue = CreateWindowW(L"STATIC", L"25", WS_CHILD | WS_VISIBLE, 430, 210, 50, 20, hwnd, NULL, NULL, NULL);
      hImageQualitySlider = CreateWindowW(L"msctls_trackbar32", NULL, WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                                          460, 210, 220, 30, hwnd, (HMENU)ID_IMAGE_QUALITY_SLIDER, NULL, NULL);

      hImageDpiLabel = CreateWindowW(L"STATIC", L"Image DPI:", WS_CHILD | WS_VISIBLE, 330, 250, 120, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hImageDpiLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
      hImageDpi = CreateWindowW(L"EDIT", IMAGE_DPI, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                460, 250, 120, 20, hwnd, NULL, NULL, NULL);
      if (hImageDpi)
         SendMessageW(hImageDpi, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      hImageSizeLabel = CreateWindowW(L"STATIC", L"Image Size:", WS_CHILD | WS_VISIBLE, 330, 280, 120, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hImageSizeLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
      hImageSize = CreateWindowW(L"EDIT", IMAGE_SIZE, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                 460, 280, 120, 20, hwnd, NULL, NULL, NULL);
      if (hImageSize)
         SendMessageW(hImageSize, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      SendMessageW(hImageQualitySlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
      SendMessageW(hImageQualitySlider, TBM_SETTICFREQ, 5, 0);
      SendMessageW(hImageQualitySlider, TBM_SETPOS, TRUE, _wtoi(IMAGE_QUALITY));

      // **Output Group (Right)**
      hOutputGroup = CreateWindowW(L"BUTTON", L"Output", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                   320, 400, 480, 100, hwnd, NULL, NULL, NULL);

      hOutputTypeLabel = CreateWindowW(L"STATIC", L"Format:", WS_CHILD | WS_VISIBLE, 330, 250, 80, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hOutputTypeLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
      hOutputType = CreateWindowExW(
          0L,
          L"COMBOBOX",
          NULL,
          WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
          460, 280, 120, 100,
          hwnd,
          (HMENU)ID_OUTPUT_TYPE, // 👈 Add the control ID here
          g_hInstance,
          NULL);

      SendMessageW(hOutputType, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      for (int i = 0; i < ARRAYSIZE(controls); ++i)
      {
         *controls[i].hCheckbox = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 350, controls[i].y, 20, 20, hwnd, NULL, NULL, NULL);
         *controls[i].hLabel = CreateWindowW(L"STATIC", controls[i].labelText, WS_CHILD | WS_VISIBLE | SS_NOTIFY, 330, controls[i].y, 180, 20, hwnd, NULL, NULL, NULL);

         SetProp(*controls[i].hLabel, L"CheckboxHandle", (HANDLE)(*controls[i].hCheckbox));
         SendMessageW(*controls[i].hLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);

         WNDPROC orig = (WNDPROC)SetWindowLongPtr(*controls[i].hLabel, GWLP_WNDPROC, (LONG_PTR)LabelProc);
         SetProp(*controls[i].hLabel, L"OrigProc", (HANDLE)orig);
         SendMessageW(*controls[i].hLabel, WM_SETFONT, (WPARAM)hFontInput, TRUE);
      }

      load_config_values(hTmpFolder, hOutputFolder, hWinrarPath, hSevenZipPath, hImageMagickPath, hImageDpi,
                         hImageSize, hImageQualityValue, hImageQualitySlider, hOutputRunImageOptimizer, 
                         hOutputRunCompressor, hOutputKeepExtracted, hOutputType, ARRAYSIZE(controls));

      SetWindowTextW(hImageQualityValue, IMAGE_QUALITY);

      InvalidateRect(hwnd, NULL, TRUE); // Ensure background updates
      UpdateWindow(hwnd);               // Force immediate redraw
      break;

   case WM_SIZE:
      GetClientRect(hwnd, &rect);

      // **Resize and Move 'Files' Group**
      MoveWindow(hRemoveButton, 20, 30, 32, 32, TRUE);
      MoveWindow(hAddButton, 55, 30, 32, 32, TRUE);
      MoveWindow(hFilesGroup, 10, 10, rect.right - 380, rect.bottom - 175, TRUE);
      MoveWindow(hListBox, 20, 70, rect.right - 400, rect.bottom - 240, TRUE);
      MoveWindow(hTerminalGroup, 10, rect.bottom - 150, rect.right - 380, 100, TRUE);
      MoveWindow(hTerminalProcessingLabel, 20, rect.bottom - 125, 100, 20, TRUE);
      MoveWindow(hTerminalProcessingText, 120, rect.bottom - 125, 100, 20, TRUE);
      MoveWindow(hTerminalText, 20, rect.bottom - 100, rect.right - 400, 46, TRUE);
      MoveWindow(hStartButton, rect.right - 460, rect.bottom - 40, 70, 30, TRUE);
      MoveWindow(hStopButton, rect.right - 560, rect.bottom - 40, 70, 30, TRUE);
      // MoveWindow(hStartButton, 20, rect.bottom - 50, 120, 30, TRUE);

      // **Keep Right Section Fixed, Move Elements Together**
      MoveWindow(hSettingsGroup, rect.right - 360, 10, 350, 190, TRUE);
      MoveWindow(hImageSettingsGroup, rect.right - 360, 210, 350, 180, TRUE);
      MoveWindow(hOutputGroup, rect.right - 360, 400, 350, 120, TRUE);

      // **Settings Fields and Labels**
      MoveWindow(hTmpFolderLabel, rect.right - 350, 40, 100, 20, TRUE);
      MoveWindow(hTmpFolder, rect.right - 240, 40, 180, 20, TRUE);
      MoveWindow(hTmpBrowse, rect.right - 50, 32, 30, 30, TRUE);

      MoveWindow(hOutputFolderLabel, rect.right - 350, 72, 100, 20, TRUE);
      MoveWindow(hOutputFolder, rect.right - 240, 72, 180, 20, TRUE);
      MoveWindow(hOutputBrowse, rect.right - 50, 65, 30, 30, TRUE);

      MoveWindow(hWinrarLabel, rect.right - 350, 105, 100, 20, TRUE);
      MoveWindow(hWinrarPath, rect.right - 240, 105, 180, 20, TRUE);
      MoveWindow(hWinrarBrowse, rect.right - 50, 98, 30, 30, TRUE);

      MoveWindow(hSevenZipLabel, rect.right - 350, 138, 100, 20, TRUE);
      MoveWindow(hSevenZipPath, rect.right - 240, 138, 180, 20, TRUE);
      MoveWindow(hSevenZipBrowse, rect.right - 50, 131, 30, 30, TRUE);

      MoveWindow(hImageMagickLabel, rect.right - 350, 171, 100, 20, TRUE);
      MoveWindow(hImageMagickPath, rect.right - 240, 171, 180, 20, TRUE);
      MoveWindow(hImageMagickBrowse, rect.right - 50, 164, 30, 30, TRUE);

      // **Image Settings Fields**
      MoveWindow(hImageQualityLabel, rect.right - 350, 240, 100, 20, TRUE);
      MoveWindow(hImageQualityValue, rect.right - 250, 240, 100, 20, TRUE);
      MoveWindow(hImageQualitySlider, rect.right - 350, 265, 330, 30, TRUE);
      MoveWindow(hImageDpiLabel, rect.right - 350, 320, 90, 20, TRUE);
      MoveWindow(hImageDpi, rect.right - 240, 320, 120, 20, TRUE);
      MoveWindow(hImageSizeLabel, rect.right - 350, 350, 90, 20, TRUE);
      MoveWindow(hImageSize, rect.right - 240, 350, 120, 20, TRUE);

      // **Output Settings Fields**
      MoveWindow(hOutputTypeLabel, rect.right - 350, 430, 80, 20, TRUE);
      MoveWindow(hOutputType, rect.right - 280, 425, 120, 20, TRUE);
      MoveWindow(hOutputRunImageOptimizer, rect.right - 350, 460, 20, 20, TRUE);
      MoveWindow(hOutputRunImageOptimizerLabel, rect.right - 330, 462, 140, 20, TRUE);
      MoveWindow(hOutputRunCompressor, rect.right - 150, 460, 20, 20, TRUE);
      MoveWindow(hOutputRunCompressorLabel, rect.right - 130, 462, 110, 20, TRUE);
      MoveWindow(hOutputKeepExtracted, rect.right - 350, 480, 20, 20, TRUE);
      MoveWindow(hOutputKeepExtractedLabel, rect.right - 330, 482, 150, 20, TRUE);

      InvalidateRect(hwnd, NULL, TRUE);
      break;

   case WM_COMMAND:
      wchar_t iniPath[MAX_PATH];
      GetModuleFileNameW(NULL, iniPath, MAX_PATH);
      wchar_t *lastSlash = wcsrchr(iniPath, '\\');
      if (lastSlash)
         *(lastSlash + 1) = '\0';
      wcscat(iniPath, L"config.ini");

      if (LOWORD(wParam) == ID_TMP_FOLDER_BROWSE)
      {
         BrowseFolder(hwnd, TMP_FOLDER);
         WritePrivateProfileStringW(L"Paths", L"TMP_FOLDER", TMP_FOLDER, iniPath);
         SetWindowTextW(hTmpFolder, TMP_FOLDER);
      }
      else if (LOWORD(wParam) == ID_OUTPUT_FOLDER_BROWSE)
      {
         BrowseFolder(hwnd, OUTPUT_FOLDER);
         WritePrivateProfileStringW(L"Paths", L"OUTPUT_FOLDER", OUTPUT_FOLDER, iniPath);
         SetWindowTextW(hOutputFolder, OUTPUT_FOLDER);
      }
      else if (LOWORD(wParam) == ID_WINRAR_PATH_BROWSE)
      {
         BrowseFile(hwnd, WINRAR_PATH);
         WritePrivateProfileStringW(L"Paths", L"WINRAR_PATH", WINRAR_PATH, iniPath);
         SetWindowTextW(hWinrarPath, WINRAR_PATH);
         update_output_type_dropdown(hOutputType, WINRAR_PATH); // clean, direct
      }
      else if (LOWORD(wParam) == ID_SEVEN_ZIP_PATH_BROWSE)
      {
         BrowseFile(hwnd, SEVEN_ZIP_PATH);
         WritePrivateProfileStringW(L"Paths", L"SEVEN_ZIP_PATH", SEVEN_ZIP_PATH, iniPath);
         SetWindowTextW(hSevenZipPath, SEVEN_ZIP_PATH);
      }
      else if (LOWORD(wParam) == ID_IMAGEMAGICK_PATH_BROWSE)
      {
         BrowseFile(hwnd, IMAGEMAGICK_PATH);
         WritePrivateProfileStringW(L"Paths", L"IMAGEMAGICK_PATH", IMAGEMAGICK_PATH, iniPath);
         SetWindowTextW(hImageMagickPath, IMAGEMAGICK_PATH);
      }
      else if (HIWORD(wParam) == EN_KILLFOCUS)
      {
         if ((HWND)lParam == hTmpFolder)
         {
            GetWindowTextW(hTmpFolder, TMP_FOLDER, MAX_PATH);
            WritePrivateProfileStringW(L"Paths", L"TMP_FOLDER", L"", iniPath);
         }
         else if ((HWND)lParam == hOutputFolder)
         {
            GetWindowTextW(hOutputFolder, OUTPUT_FOLDER, MAX_PATH);
            WritePrivateProfileStringW(L"Paths", L"OUTPUT_FOLDER", L"", iniPath);
         }
         else if ((HWND)lParam == hWinrarPath)
         {
            GetWindowTextW(hWinrarPath, WINRAR_PATH, MAX_PATH);
            WritePrivateProfileStringW(L"Paths", L"WINRAR_PATH", L"", iniPath);

            update_output_type_dropdown(hOutputType, WINRAR_PATH); // clean, direct
         }
         else if ((HWND)lParam == hSevenZipPath)
         {
            GetWindowTextW(hSevenZipPath, SEVEN_ZIP_PATH, MAX_PATH);
            WritePrivateProfileStringW(L"Paths", L"SEVEN_ZIP_PATH", L"", iniPath);
         }
         else if ((HWND)lParam == hImageMagickPath)
         {
            GetWindowTextW(hImageMagickPath, IMAGEMAGICK_PATH, MAX_PATH);
            WritePrivateProfileStringW(L"Paths", L"IMAGEMAGICK_PATH", L"", iniPath);
         }
      }
      // In WndProc → WM_COMMAND
      else if (LOWORD(wParam) == ID_START_BUTTON)
      {
         g_StopProcessing = FALSE;
         CreateThread(NULL, 0, ProcessingThread, hwnd, 0, NULL);
      }

      else if (LOWORD(wParam) == ID_STOP_BUTTON)
      {
         g_StopProcessing = TRUE;
      }
      else if (LOWORD(wParam) == ID_ADD_BUTTON)
      {
         OpenFileDialog(hwnd, hListBox);
      }
      else if (LOWORD(wParam) == ID_REMOVE_BUTTON) // Remove VK_DELETE and VK_BACK check
      {
         RemoveSelectedItems(hListBox);
      }
      else if (LOWORD(wParam) == ID_HELP_ABOUT)
      {
         ShowAboutWindow(hwnd, g_hInstance);
      }
      else if (LOWORD(wParam) == ID_FILE_EXIT)
      {
         DestroyWindow(hwnd);
      }

      // Save checkbox state when any of them is toggled
      for (int i = 0; i < controlCount; ++i)
      {
         if ((HWND)lParam == *controls[i].hCheckbox && HIWORD(wParam) == BN_CLICKED)
         {
            LRESULT checked = SendMessageW(*controls[i].hCheckbox, BM_GETCHECK, 0, 0);
            WritePrivateProfileStringW(L"Output", controls[i].configKey,
                                       (checked == BST_CHECKED) ? L"true" : L"false",
                                       iniPath);
            break; // handled the toggle, done
         }
      }

      break;

   case WM_LBUTTONDOWN:
      SetFocus(hListBox);
      break;

   case ID_IMAGE_QUALITY_SLIDER:
   {
      int sliderPos = SendMessageW(hImageQualitySlider, TBM_GETPOS, 0, 0);

      // Update IMAGE_QUALITY string
      swprintf(IMAGE_QUALITY, 16, L"%d", sliderPos);

      SetWindowTextW(hImageQualityValue, IMAGE_QUALITY);

      // Write to INI
      wchar_t iniPath[MAX_PATH];
      GetModuleFileNameW(NULL, iniPath, MAX_PATH);
      wchar_t *lastSlash = wcsrchr(iniPath, L'\\');
      if (lastSlash)
         *(lastSlash + 1) = '\0';
      wcscat(iniPath, L"config.ini");

      WritePrivateProfileStringW(L"Image", L"IMAGE_QUALITY", IMAGE_QUALITY, iniPath);
   }
   break;
   case WM_HSCROLL:
   {
      if ((HWND)lParam == hImageQualitySlider)
      {
         int sliderPos = SendMessageW(hImageQualitySlider, TBM_GETPOS, 0, 0);
         swprintf(IMAGE_QUALITY, 16, L"%d", sliderPos);
         SetWindowTextW(hImageQualityValue, IMAGE_QUALITY);

         // Save to INI
         wchar_t iniPath[MAX_PATH];
         GetModuleFileNameW(NULL, iniPath, MAX_PATH);
         wchar_t *lastSlash = wcsrchr(iniPath, L'\\');
         if (lastSlash)
            *(lastSlash + 1) = '\0';
         wcscat(iniPath, L"config.ini");

         WritePrivateProfileStringW(L"Image", L"IMAGE_QUALITY", IMAGE_QUALITY, iniPath);
      }
   }
   break;

   case WM_DROPFILES:
      ProcessDroppedFiles(hwnd, hListBox, (HDROP)wParam);
      break;

   case WM_CLOSE:
      DestroyWindow(hwnd);
      break;

   case WM_DESTROY:
      DeleteObject(hGrayBrush);
      PostQuitMessage(0);
      break;

      // For BLENDFUNCTION and AlphaBlend

   case WM_DRAWITEM:
   {
      LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
      HBITMAP hBmp = NULL;
      BOOL isHover = FALSE;
      int bmpW = 32, bmpH = 32; // Default size

      // Select the appropriate bitmap and hover state
      if (lpdis->CtlID == ID_REMOVE_BUTTON)
      {
         hBmp = hButtonMinus;
         isHover = isHoverRemove;
      }
      else if (lpdis->CtlID == ID_ADD_BUTTON)
      {
         hBmp = hButtonPlus;
         isHover = isHoverAdd;
      }
      else if (lpdis->CtlID == ID_START_BUTTON)
      {
         hBmp = hButtonStart;
         bmpW = 70;
         bmpH = 30;
         isHover = isHoverStart;
      }
      else if (lpdis->CtlID == ID_TMP_FOLDER_BROWSE || lpdis->CtlID == ID_OUTPUT_FOLDER_BROWSE ||
               lpdis->CtlID == ID_IMAGEMAGICK_PATH_BROWSE || lpdis->CtlID == ID_SEVEN_ZIP_PATH_BROWSE ||
               lpdis->CtlID == ID_WINRAR_PATH_BROWSE)
      {
         hBmp = hButtonBrowse;
         bmpW = 26;
         bmpH = 26;
         isHover = isBrowseButtonHovered(lpdis->CtlID);
      }

      if (hBmp)
      {
         HDC hdcMem = CreateCompatibleDC(lpdis->hDC);
         HGDIOBJ oldBmp = SelectObject(hdcMem, hBmp);

         // Apply hover effect correctly to hButtonStart
         if (isHover)
         {
            bmpW -= 2;
            bmpH -= 2;
         }

         // Center the bitmap in the button
         int btnW = lpdis->rcItem.right - lpdis->rcItem.left;
         int btnH = lpdis->rcItem.bottom - lpdis->rcItem.top;
         int x = (btnW - bmpW) / 2;
         int y = (btnH - bmpH) / 2;

         // Create a new compatible memory DC for scaling
         HDC hdcScaled = CreateCompatibleDC(lpdis->hDC);
         HBITMAP hScaledBmp = CreateCompatibleBitmap(lpdis->hDC, bmpW, bmpH);
         HGDIOBJ oldScaledBmp = SelectObject(hdcScaled, hScaledBmp);

         // Scale while respecting original size
         StretchBlt(hdcScaled, 0, 0, bmpW, bmpH, hdcMem, 0, 0, (lpdis->CtlID == ID_START_BUTTON ? 70 : 32), (lpdis->CtlID == ID_START_BUTTON ? 30 : 32), SRCCOPY);

         // Transparent rendering
         TransparentBlt(lpdis->hDC, x, y, bmpW, bmpH, hdcScaled, 0, 0, bmpW, bmpH, RGB(255, 255, 255));

         // Cleanup
         SelectObject(hdcScaled, oldScaledBmp);
         DeleteObject(hScaledBmp);
         DeleteDC(hdcScaled);
         SelectObject(hdcMem, oldBmp);
         DeleteDC(hdcMem);
      }

      return TRUE;
   }

   case WM_CTLCOLORSTATIC:
   {
      HDC hdcStatic = (HDC)wParam;
      HWND hStatic = (HWND)lParam;

      // List of transparent controls
      HWND transparentControls[] = {hTmpFolderLabel, hOutputFolderLabel, hWinrarLabel, hSevenZipLabel, hImageMagickLabel, hImageQualityLabel, hImageDpiLabel};
      int transparentCount = sizeof(transparentControls) / sizeof(transparentControls[0]);

      // Check if the control is in the transparency list
      for (int i = 0; i < transparentCount; i++)
      {
         if (hStatic == transparentControls[i])
         {
            SetBkMode(hdcStatic, TRANSPARENT);
            SetTextColor(hdcStatic, RGB(0, 0, 0)); // Ensure text remains visible
            return (LRESULT)GetStockObject(NULL_BRUSH);
         }
      }

      if (hStatic == hTerminalProcessingText || hStatic == hTerminalProcessingLabel)
      {
         SetTextColor(hdcStatic, RGB(0, 128, 0)); // Dark green, for example
         SetBkMode(hdcStatic, OPAQUE);
         SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE)); // or COLOR_WINDOW
         return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
      }

      return DefWindowProc(hwnd, msg, wParam, lParam); // Default behavior for others
   }

   case WM_SETFOCUS:
      // Move caret to end and scroll to it
      if (hwnd == hTmpFolder || hwnd == hOutputFolder || hwnd == hWinrarPath || hwnd == hSevenZipPath || hwnd == hImageMagickPath)
      {
         int textLength = GetWindowTextLength(hwnd);
         SendMessage(hwnd, EM_SETSEL, textLength, textLength); // Move caret to end
         SendMessage(hwnd, EM_SCROLLCARET, 0, 0);              // Ensure caret is visible
      }
      break;

   case WM_UPDATE_TERMINAL_TEXT:
   {
      const wchar_t *msg = (const wchar_t *)lParam;
      SetWindowTextW(hTerminalText, msg);
      free((void *)msg);
      return 0;
   }

   case WM_UPDATE_PROCESSING_TEXT:
   {
      const wchar_t *msg = (const wchar_t *)lParam;
      SetWindowTextW(hTerminalProcessingText, msg);

      InvalidateRect(hTerminalProcessingText, NULL, TRUE); // force repaint
      UpdateWindow(hTerminalProcessingText);               // redraw immediately

      free((void *)msg);
      return 0;
   }
   }
   return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
   g_hInstance = hInstance;
   INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES};
   icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
   InitCommonControlsEx(&icex);

   WNDCLASSEXW wc = {0};
   wc.cbSize = sizeof(wc); // Required for extended class structure
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = WndProc;
   wc.hInstance = hInstance;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.lpszClassName = L"ResizableWindowClass";
   wc.hInstance = hInstance;
   wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
   // wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // To change BCKG and test transparency of components

   // **Set both large and small icons**
   wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, LR_SHARED);
   wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_SHARED);

   if (!RegisterClassExW(&wc))
      return -1;

   HWND hwnd = CreateWindowW(L"ResizableWindowClass", L"CBRZ Optimizer", WS_OVERLAPPEDWINDOW | WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT,
                             500, 400, NULL, NULL, hInstance, NULL);

   SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
   SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   if (!IsThemeActive())
   {
      MessageBoxW(NULL, L"Visual styles are NOT active!", L"Theme Status", MB_OK | MB_ICONWARNING);
   }

   // **Handle dragged files when launching the .exe (Skip empty entries)**
   if (wcslen(lpCmdLine) > 0)
   {
      wchar_t filePath[MAX_PATH];
      wchar_t *token = wcstok(lpCmdLine, L"\"", NULL); // Unicode-safe tokenization

      while (token)
      {
         // Skip entries that are too short
         if (wcslen(token) > 2)
         {
            wcscpy(filePath, token); // Unicode-safe copy
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)filePath);
         }
         token = wcstok(NULL, L"\"", NULL);
      }
   }

   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return msg.wParam;
}
