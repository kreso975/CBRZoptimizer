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
#include "window.h"
#include "resource.h"
#include "gui.h"
#include "functions.h"
#include "aboutDialog.h"
#include "debug.h"

#include <uxtheme.h>

HINSTANCE g_hInstance;

HFONT hBoldFont, hFontLabel, hFontInput, hFontEmoji;
HBITMAP hButtonPlus, hButtonMinus, hButtonBrowse, hButtonStart, hButtonStop;
// Main controls
HWND hListBox, hStartButton, hStopButton, hAddButton, hRemoveButton, hLabelNumberOfFiles, hNumberOfFiles, hSettingsWnd;
// Paths
HWND hTmpFolder, hOutputFolder, hWinrarPath, hSevenZipPath, hImageMagickPath, hImageResize;
HWND hTmpBrowse, hOutputBrowse, hWinrarBrowse, hSevenZipBrowse, hImageMagickBrowse;
// Groups
HWND hFilesGroup, hTerminalGroup, hSettingsGroup, hImageSettingsGroup, hOutputGroup;
// Terminal
HWND hTerminalProcessingLabel, hTerminalProcessingText, hTerminalText;
// Labels
HWND hTmpFolderLabel, hOutputFolderLabel, hWinrarLabel, hSevenZipLabel;
HWND hImageTypeLabel, hImageAllowUpscalingLabel, hImageResizeToLabel, hImageMagickLabel;
HWND hImageQualityLabel, hImageQualityValue, hImageSizeWidthLabel, hImageSizeHeightLabel, hImageKeepAspectRatioLabel;
// Image settings
HWND hImageType, hImageAllowUpscaling, hImageResizeTo, hImageQualitySlider, hImageSizeWidth, hImageSizeHeight, hImageKeepAspectRatio;
// Output options
HWND hOutputKeepExtractedLabel, hOutputKeepExtracted, hOutputRunExtractLabel, hOutputRunExtract;
HWND hOutputType, hOutputTypeLabel, hOutputRunImageOptimizer, hOutputRunCompressor, hOutputRunImageOptimizerLabel, hOutputRunCompressorLabel;

AppConfig g_config;

volatile BOOL g_StopProcessing = FALSE;

// define + initialize all defaults in one shot
AppConfig g_config = {
    // Paths default to empty strings
    .TMP_FOLDER = L"",
    .OUTPUT_FOLDER = L"",
    .WINRAR_PATH = L"",
    .IMAGEMAGICK_PATH = L"",
    .SEVEN_ZIP_PATH = L"",

    // Image defaults
    .IMAGE_TYPE = L"",
    .resizeTo = FALSE,
    .keepAspectRatio = FALSE,
    .IMAGE_SIZE_WIDTH = L"",
    .IMAGE_SIZE_HEIGHT = L"",
    .IMAGE_QUALITY = L"",

    // Output defaults
    .runImageOptimizer = TRUE,
    .runCompressor = TRUE,
    .keepExtracted = TRUE};

extern LabelCheckboxPair controls[];
extern const int controlCount;

GUIHandleEntry groupElements[] = {
    {L"ListBox", L"FilesGroup", &hListBox},
    {L"AddButton", L"FilesGroup", &hAddButton},
    {L"RemoveButton", L"FilesGroup", &hRemoveButton},

    {L"TmpFolder", L"PathsGroup", &hTmpFolder},
    {L"TmpFolder Label", L"PathsGroup", &hTmpFolderLabel},
    {L"TmpFolder Browse", L"PathsGroup", &hTmpBrowse},
    {L"OutputFolder", L"PathsGroup", &hOutputFolder},
    {L"OutputFolder Label", L"PathsGroup", &hOutputFolderLabel},
    {L"OutputFolder Browse", L"PathsGroup", &hOutputBrowse},
    {L"WinRAR Path", L"PathsGroup", &hWinrarPath},
    {L"WinRAR Label", L"PathsGroup", &hWinrarLabel},
    {L"WinRAR Browse", L"PathsGroup", &hWinrarBrowse},
    {L"7-Zip Path", L"PathsGroup", &hSevenZipPath},
    {L"7-Zip Label", L"PathsGroup", &hSevenZipLabel},
    {L"7-Zip Browse", L"PathsGroup", &hSevenZipBrowse},
    {L"ImageMagick Path", L"PathsGroup", &hImageMagickPath},
    {L"ImageMagick Label", L"PathsGroup", &hImageMagickLabel},
    {L"ImageMagick Browse", L"PathsGroup", &hImageMagickBrowse},

    {L"Image Resize", L"ImageGroup", &hImageResize},
    {L"ImageTypeLabel", L"ImageGroup", &hImageTypeLabel},
    {L"hImageAllowUpscalingLabel", L"ImageGroup", &hImageAllowUpscalingLabel},
    {L"hImageResizeToLabel", L"ImageGroup", &hImageResizeToLabel},
    {L"hImageQualityLabel", L"ImageGroup", &hImageQualityLabel},
    {L"hImageQualityValue", L"ImageGroup", &hImageQualityValue},
    {L"hImageSizeWidthLabel", L"ImageGroup", &hImageSizeWidthLabel},
    {L"hImageSizeHeightLabel", L"ImageGroup", &hImageSizeHeightLabel},
    {L"hImageKeepAspectRatioLabel", L"ImageGroup", &hImageKeepAspectRatioLabel},
    {L"hImageKeepAspectRatio", L"ImageGroup", &hImageKeepAspectRatio},
    {L"hImageType", L"ImageGroup", &hImageType},
    {L"hImageAllowUpscaling", L"ImageGroup", &hImageAllowUpscaling},
    {L"hImageResizeTo", L"ImageGroup", &hImageResizeTo},
    {L"hImageQualitySlider", L"ImageGroup", &hImageQualitySlider},
    {L"hImageSizeWidth", L"ImageGroup", &hImageSizeWidth},
    {L"hImageSizeHeight", L"ImageGroup", &hImageSizeHeight},

    {L"hOutputKeepExtractedLabel", L"OutputGroup", &hOutputKeepExtractedLabel},
    {L"hOutputKeepExtracted", L"OutputGroup", &hOutputKeepExtracted},
    {L"hOutputRunExtractLabel", L"OutputGroup", &hOutputRunExtractLabel},
    {L"hOutputRunExtract", L"OutputGroup", &hOutputRunExtract},
    {L"hOutputType", L"OutputGroup", &hOutputType},
    {L"hOutputTypeLabel", L"OutputGroup", &hOutputTypeLabel},
    {L"hOutputRunImageOptimizer", L"OutputGroup", &hOutputRunImageOptimizer},
    {L"hOutputRunImageOptimizerLabel", L"OutputGroup", &hOutputRunImageOptimizerLabel},
    {L"hOutputRunCompressor", L"OutputGroup", &hOutputRunCompressor},
    {L"hOutputRunCompressorLabel", L"OutputGroup", &hOutputRunCompressorLabel}};

int groupElementsCount = sizeof(groupElements) / sizeof(groupElements[0]);

const ImageTypeEntry g_ImageTypeOptions[] = {
    {L"Portrait", IMAGE_TYPE_PORTRAIT},
    {L"Landscape", IMAGE_TYPE_LANDSCAPE}};

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

// Refresh Window after disabling
void AdjustLayout(HWND hwnd)
{
   RECT rect;
   GetWindowRect(hwnd, &rect);
   SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOZORDER | SWP_NOMOVE | SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_FRAMECHANGED);
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
         LRESULT state = SendMessageW(hCheckbox, BM_GETCHECK, 0, 0);
         LRESULT newState = (state == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED;
         SendMessageW(hCheckbox, BM_SETCHECK, (WPARAM)newState, 0);

         // Build config path
         wchar_t iniPath[MAX_PATH];
         GetModuleFileNameW(NULL, iniPath, MAX_PATH);
         wchar_t *lastSlash = wcsrchr(iniPath, L'\\');
         if (lastSlash)
            *(lastSlash + 1) = L'\0';
         wcscat(iniPath, L"config.ini");

         for (int i = 0; i < controlCount; ++i)
         {
            if (hCheckbox == *controls[i].hCheckbox)
            {
               const wchar_t *section = controls[i].configSegment;
               const wchar_t *key = controls[i].configKey;
               const wchar_t *value = (newState == BST_CHECKED) ? L"true" : L"false";

               WritePrivateProfileStringW(section, key, value, iniPath);

               // Update g_config if needed
               if (wcscmp(key, L"hOutputRunImageOptimizer") == 0)
                  g_config.runImageOptimizer = (newState == BST_CHECKED);
               else if (wcscmp(key, L"hOutputRunCompressor") == 0)
                  g_config.runCompressor = (newState == BST_CHECKED);
               else if (wcscmp(key, L"hOutputKeepExtracted") == 0)
                  g_config.keepExtracted = (newState == BST_CHECKED);
               else if (wcscmp(key, L"IMAGE_RESIZE_TO") == 0)
                  g_config.resizeTo = (newState == BST_CHECKED);
               else if (wcscmp(key, L"IMAGE_KEEP_ASPECT_RATIO") == 0)
                  g_config.keepAspectRatio = (newState == BST_CHECKED);
               else if (wcscmp(key, L"IMAGE_ALLOW_UPSCALING") == 0)
                  g_config.allowUpscaling = (newState == BST_CHECKED);
               break;
            }
         }

         if (hwnd == hImageResizeToLabel || hwnd == hImageKeepAspectRatioLabel || hwnd == hOutputRunImageOptimizerLabel)
         {
            BOOL shouldEnable = g_config.runImageOptimizer;

            EnableResizeGroupWithLogic(L"ImageGroup", shouldEnable);

            // If the group is visible and needs layout attention
            if (shouldEnable)
            {
               AdjustLayout(GetParent(hwnd)); // VERY IMPORTANT: GetParent(hwnd) only this works for Label
            }
         }
      }
   }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
   WNDPROC orig = (WNDPROC)GetProp(hwnd, L"OrigProc");
#pragma GCC diagnostic pop
   return CallWindowProc(orig ? orig : DefWindowProc, hwnd, msg, wParam, lParam);
}

BOOL isHoverRemove = FALSE;
BOOL isHoverAdd = FALSE;
BOOL isHoverStart = FALSE;
BOOL isHoverStop = FALSE;
BOOL isHoverTmp = FALSE;
BOOL isHoverOutput = FALSE;
BOOL isHoverWinrar = FALSE;
BOOL isHoverSevenZip = FALSE;
BOOL isHoverImageMagick = FALSE;

LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static TRACKMOUSEEVENT tme;
   int ctlId = GetDlgCtrlID(hwnd);

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
   case ID_STOP_BUTTON:
      hoverFlag = &isHoverStop;
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

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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

      static HBRUSH hGrayBrush = NULL;
      hGrayBrush = CreateSolidBrush(RGB(192, 192, 192));

      int defaultWidth = 960;  // 800 * 1.2 (20% increase)
      int defaultHeight = 600; // Keep height unchanged unless needed
      MoveWindow(hwnd, 100, 100, defaultWidth, defaultHeight, TRUE);

      HMENU hMenu = CreateMenu();
      HMENU menuFile = CreatePopupMenu();
      HMENU helpAboutMenu = CreatePopupMenu();

      AppendMenuW(menuFile, MF_STRING, ID_FILE_EXIT, L"Exit");
      AppendMenuW(helpAboutMenu, MF_STRING, ID_HELP_ABOUT, L"About");

      AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)menuFile, L"File");
      AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)helpAboutMenu, L"Help");

      // Correct way to right-align "Help"
      MENUITEMINFO mii = {0};
      mii.cbSize = sizeof(MENUITEMINFO);
      mii.fMask = MIIM_FTYPE;
      mii.fType = MFT_RIGHTJUSTIFY;

      SetMenuItemInfo(hMenu, (UINT)(GetMenuItemCount(hMenu) - 1), TRUE, &mii);

      SetMenu(hwnd, hMenu);

      // **Files Group (Left)**
      hFilesGroup = CreateWindowW(L"BUTTON", L"Files", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                  10, 10, 300, 200, hwnd, NULL, NULL, NULL);

      // Create static controls for images (borderless)
      hRemoveButton = CreateWindowW(
          L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
          20, 30, 32, 32, hwnd, (HMENU)ID_REMOVE_BUTTON, NULL, NULL);

      hAddButton = CreateWindowW(
          L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
          70, 30, 32, 32, hwnd, (HMENU)ID_ADD_BUTTON, NULL, NULL);

      // Load BMP images
      hButtonPlus = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_BUTTON_PLUS));
      hButtonMinus = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_BUTTON_MINUS));

      if (!hButtonPlus)
      {
         MessageBeep(MB_ICONERROR); // Play error sound
         MessageBoxCentered(hwnd, L"Failed to load plus image!", L"Error", MB_OK | MB_ICONERROR);
      }
      else
         SendMessageW(hAddButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hButtonPlus);

      if (!hButtonMinus)
      {
         MessageBeep(MB_ICONERROR); // Play error sound
         MessageBoxCentered(hwnd, L"Failed to load minus image!", L"Error", MB_OK | MB_ICONERROR);
      }
      else
         SendMessageW(hRemoveButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hButtonMinus);

      SetWindowLongPtr(hRemoveButton, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(hRemoveButton, GWLP_WNDPROC));
      SetWindowLongPtr(hRemoveButton, GWLP_WNDPROC, (LONG_PTR)ButtonProc);

      SetWindowLongPtr(hAddButton, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(hAddButton, GWLP_WNDPROC));
      SetWindowLongPtr(hAddButton, GWLP_WNDPROC, (LONG_PTR)ButtonProc);

      // **Listbox settings**
      hListBox = CreateWindowW(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
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
         SendMessageW(hTerminalText, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      hStartButton = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
                                   20, 230, 70, 30, hwnd, (HMENU)ID_START_BUTTON, NULL, NULL);

      hButtonStart = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_BUTTON_START));
      hButtonStop = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_BUTTON_STOP));

      if (!hButtonStart)
      {
         MessageBeep(MB_ICONERROR); // Play error sound
         MessageBoxCentered(hwnd, L"Failed to load start image!", L"Error", MB_OK | MB_ICONERROR);
      }
      else
         SendMessageW(hStartButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hButtonStart);

      SetWindowLongPtr(hStartButton, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(hStartButton, GWLP_WNDPROC));
      SetWindowLongPtr(hStartButton, GWLP_WNDPROC, (LONG_PTR)ButtonProc);

      hStopButton = CreateWindowW(L"BUTTON", L"Stop", WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
                                  20, 230, 70, 30, hwnd, (HMENU)ID_STOP_BUTTON, NULL, NULL);

      if (!hButtonStop)
      {
         MessageBeep(MB_ICONERROR); // Play error sound
         MessageBoxCentered(hwnd, L"Failed to load stop image!", L"Error", MB_OK | MB_ICONERROR);
      }
      else
         SendMessageW(hStopButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hButtonStop);

      SetWindowLongPtr(hStopButton, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(hStopButton, GWLP_WNDPROC));
      SetWindowLongPtr(hStopButton, GWLP_WNDPROC, (LONG_PTR)ButtonProc);

      // **Settings Group (Right)**
      hSettingsGroup = CreateWindowW(L"BUTTON", L"Settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                     320, 10, 480, 210, hwnd, NULL, NULL, NULL);

      hButtonBrowse = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_BUTTON_ADD));

      for (size_t i = 0; i < inputsCount; ++i)
      {
         *inputs[i].hLabel = CreateWindowW(L"STATIC", inputs[i].labelText, WS_CHILD | WS_VISIBLE, 20, inputs[i].y, inputs[i].labelWidth, 20, hwnd, NULL, NULL, NULL);
         *inputs[i].hEdit = CreateWindowW(L"EDIT", inputs[i].defaultText, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 120, inputs[i].y, inputs[i].editWidth, 20, hwnd, NULL, NULL, NULL);
         *inputs[i].hBrowse = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 320, inputs[i].y, 32, 32, hwnd, (HMENU)(UINT_PTR)inputs[i].browseId, NULL, NULL);

         if (inputs[i].hBitmap && *inputs[i].hBitmap)
         {
            SendMessageW(*inputs[i].hBrowse, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)*inputs[i].hBitmap);
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
      hImageQualitySlider = CreateWindowW(L"msctls_trackbar32", NULL, WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | WS_TABSTOP,
                                          460, 210, 220, 30, hwnd, (HMENU)ID_IMAGE_QUALITY_SLIDER, NULL, NULL);

      SendMessageW(hImageQualitySlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
      SendMessageW(hImageQualitySlider, TBM_SETTICFREQ, 5, 0);
      SendMessageW(hImageQualitySlider, TBM_SETPOS, TRUE, _wtoi(g_config.IMAGE_QUALITY));

      hImageTypeLabel = CreateWindowW(L"STATIC", L"Orientation:", WS_CHILD | WS_VISIBLE, 330, 250, 80, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hImageTypeLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
      hImageType = CreateWindowExW(0L, L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                                   460, 280, 120, 100, hwnd,
                                   (HMENU)ID_IMAGE_TYPE, // 👈 Add the control ID here
                                   g_hInstance, NULL);

      SendMessageW(hImageType, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      for (int i = 0; i < IMAGE_TYPE_COUNT; ++i)
         SendMessageW(hImageType, CB_ADDSTRING, 0, (LPARAM)g_ImageTypeOptions[i].label);

      // Optional: set default to first entry
      SendMessageW(hImageType, CB_SETCURSEL, 0, 0);
      // Update IMAGE_TYPE global from struct
      wcsncpy(g_config.IMAGE_TYPE, g_ImageTypeOptions[0].label, sizeof(g_config.IMAGE_TYPE) / sizeof(wchar_t) - 1);
      g_config.IMAGE_TYPE[sizeof(g_config.IMAGE_TYPE) / sizeof(wchar_t) - 1] = L'\0'; // Null-terminate

      hImageSizeWidthLabel = CreateWindowW(L"STATIC", L"Width:", WS_CHILD | WS_VISIBLE, 330, 280, 60, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hImageSizeWidthLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
      hImageSizeWidth = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
                                      460, 280, 80, 20, hwnd, NULL, NULL, NULL);
      if (hImageSizeWidth)
         SendMessageW(hImageSizeWidth, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      hImageSizeHeightLabel = CreateWindowW(L"STATIC", L"Height:", WS_CHILD | WS_VISIBLE, 330, 280, 60, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hImageSizeHeightLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
      hImageSizeHeight = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
                                       460, 280, 80, 20, hwnd, NULL, NULL, NULL);
      if (hImageSizeHeight)
         SendMessageW(hImageSizeHeight, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      // **Output Group (Right)**
      hOutputGroup = CreateWindowW(L"BUTTON", L"Output", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                   320, 400, 480, 100, hwnd, NULL, NULL, NULL);

      hOutputTypeLabel = CreateWindowW(L"STATIC", L"Format:", WS_CHILD | WS_VISIBLE, 330, 250, 80, 20, hwnd, NULL, NULL, NULL);
      SendMessageW(hOutputTypeLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
      hOutputType = CreateWindowExW(0L, L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                                    460, 280, 120, 100, hwnd, (HMENU)ID_OUTPUT_TYPE, // 👈 Add the control ID here
                                    g_hInstance, NULL);

      SendMessageW(hOutputType, WM_SETFONT, (WPARAM)hFontInput, TRUE);

      for (int i = 0; i < controlCount; ++i)
      {
         *controls[i].hCheckbox = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 350, controls[i].y, 20, 20, hwnd, NULL, NULL, NULL);
         *controls[i].hLabel = CreateWindowW(L"STATIC", controls[i].labelText, WS_CHILD | WS_VISIBLE | SS_NOTIFY, 330, controls[i].y, 180, 20, hwnd, NULL, NULL, NULL);

         SetProp(*controls[i].hLabel, L"CheckboxHandle", (HANDLE)(*controls[i].hCheckbox));
         SendMessageW(*controls[i].hLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);

         WNDPROC orig = (WNDPROC)SetWindowLongPtr(*controls[i].hLabel, GWLP_WNDPROC, (LONG_PTR)LabelProc);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
         SetProp(*controls[i].hLabel, L"OrigProc", (HANDLE)orig);
#pragma GCC diagnostic pop
         SendMessageW(*controls[i].hLabel, WM_SETFONT, (WPARAM)hFontInput, TRUE);
      }

      load_config_values();

      EnableResizeGroupWithLogic(L"ImageGroup", g_config.runImageOptimizer); // Must be last to update Controls

      InvalidateRect(hwnd, NULL, TRUE); // Ensure background updates
      UpdateWindow(hwnd);               // Force immediate redraw
      break;

   case WM_SIZE:
      GetClientRect(hwnd, &rect);

      // **Resize and Move 'Files' Group**
      MoveWindow(hRemoveButton, 20, 30, 32, 32, TRUE);
      MoveWindow(hAddButton, 55, 30, 32, 32, TRUE);
      MoveWindow(hFilesGroup, 10, 10, rect.right - 380, rect.bottom - 175, TRUE);
      MoveWindow(hListBox, 20, 70, rect.right - 400, rect.bottom - 245, TRUE);
      MoveWindow(hTerminalGroup, 10, rect.bottom - 150, rect.right - 380, 100, TRUE);
      MoveWindow(hTerminalProcessingLabel, 20, rect.bottom - 125, 100, 20, TRUE);
      MoveWindow(hTerminalProcessingText, 120, rect.bottom - 125, 100, 20, TRUE);
      MoveWindow(hTerminalText, 20, rect.bottom - 100, rect.right - 400, 46, TRUE);
      MoveWindow(hStartButton, rect.right - 460, rect.bottom - 40, 70, 30, TRUE);
      MoveWindow(hStopButton, rect.right - 460, rect.bottom - 40, 70, 30, TRUE);
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
      MoveWindow(hImageTypeLabel, rect.right - 350, 310, 90, 20, TRUE);
      MoveWindow(hImageType, rect.right - 270, 305, 120, 20, TRUE);

      MoveWindow(hImageResizeTo, rect.right - 350, 340, 20, 20, TRUE);
      MoveWindow(hImageResizeToLabel, rect.right - 330, 342, 80, 20, TRUE);

      MoveWindow(hImageSizeWidthLabel, rect.right - 245, 340, 35, 20, TRUE);
      MoveWindow(hImageSizeWidth, rect.right - 205, 340, 50, 20, TRUE);
      MoveWindow(hImageSizeHeightLabel, rect.right - 130, 340, 45, 20, TRUE);
      MoveWindow(hImageSizeHeight, rect.right - 85, 340, 50, 20, TRUE);

      MoveWindow(hImageKeepAspectRatio, rect.right - 350, 365, 20, 20, TRUE);
      MoveWindow(hImageKeepAspectRatioLabel, rect.right - 330, 367, 100, 20, TRUE);

      MoveWindow(hImageAllowUpscaling, rect.right - 150, 365, 20, 20, TRUE);
      MoveWindow(hImageAllowUpscalingLabel, rect.right - 130, 367, 100, 20, TRUE);

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
         BrowseFolder(hwnd, g_config.TMP_FOLDER);
         WritePrivateProfileStringW(L"Paths", L"TMP_FOLDER", g_config.TMP_FOLDER, iniPath);
         SetWindowTextW(hTmpFolder, g_config.TMP_FOLDER);
      }
      else if (LOWORD(wParam) == ID_OUTPUT_FOLDER_BROWSE)
      {
         BrowseFolder(hwnd, g_config.OUTPUT_FOLDER);
         WritePrivateProfileStringW(L"Paths", L"OUTPUT_FOLDER", g_config.OUTPUT_FOLDER, iniPath);
         SetWindowTextW(hOutputFolder, g_config.OUTPUT_FOLDER);
      }
      else if (LOWORD(wParam) == ID_WINRAR_PATH_BROWSE)
      {
         BrowseFile(hwnd, g_config.WINRAR_PATH);
         WritePrivateProfileStringW(L"Paths", L"WINRAR_PATH", g_config.WINRAR_PATH, iniPath);
         SetWindowTextW(hWinrarPath, g_config.WINRAR_PATH);
         update_output_type_dropdown(g_config.WINRAR_PATH); // clean, direct
      }
      else if (LOWORD(wParam) == ID_SEVEN_ZIP_PATH_BROWSE)
      {
         BrowseFile(hwnd, g_config.SEVEN_ZIP_PATH);
         WritePrivateProfileStringW(L"Paths", L"SEVEN_ZIP_PATH", g_config.SEVEN_ZIP_PATH, iniPath);
         SetWindowTextW(hSevenZipPath, g_config.SEVEN_ZIP_PATH);
      }
      else if (LOWORD(wParam) == ID_IMAGEMAGICK_PATH_BROWSE)
      {
         BrowseFile(hwnd, g_config.IMAGEMAGICK_PATH);
         WritePrivateProfileStringW(L"Paths", L"IMAGEMAGICK_PATH", g_config.IMAGEMAGICK_PATH, iniPath);
         SetWindowTextW(hImageMagickPath, g_config.IMAGEMAGICK_PATH);
      }
      else if (HIWORD(wParam) == EN_KILLFOCUS)
      {
         wchar_t buffer[MAX_PATH];
         GetWindowTextW((HWND)lParam, buffer, MAX_PATH);

         for (size_t i = 0; i < inputsCount; ++i)
         {
            if (*(inputs[i].hEdit) == (HWND)lParam)
            {
               ValidateAndSaveInput((HWND)lParam, iniPath);
               break;
            }
         }

         if ((HWND)lParam == hWinrarPath)
            update_output_type_dropdown(g_config.WINRAR_PATH);

         else if ((HWND)lParam == hImageSizeWidth)
         {
            wcscpy(g_config.IMAGE_SIZE_WIDTH, buffer);
            WritePrivateProfileStringW(L"Image", L"IMAGE_SIZE_WIDTH", buffer, iniPath);
         }
         else if ((HWND)lParam == hImageSizeHeight)
         {
            wcscpy(g_config.IMAGE_SIZE_HEIGHT, buffer);
            WritePrivateProfileStringW(L"Image", L"IMAGE_SIZE_HEIGHT", buffer, iniPath);
         }
      }
      // In WndProc → WM_COMMAND
      else if (LOWORD(wParam) == ID_START_BUTTON)
      {
         g_StopProcessing = FALSE;
         int count = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0);
         if (count <= 0)
         {
            MessageBeep(MB_ICONINFORMATION);
            MessageBoxCentered(hwnd, L"No Files selected!", L"Info", MB_OK | MB_ICONINFORMATION);
            return 0;
         }

         ShowWindow(hStartButton, SW_HIDE);
         ShowWindow(hStopButton, SW_SHOW);
         ShowWindow(hTerminalProcessingLabel, SW_SHOW);
         ShowWindow(hTerminalProcessingText, SW_SHOW);
         EnableResizeGroupWithLogic(L"FilesGroup", FALSE);
         EnableResizeGroupWithLogic(L"OutputGroup", FALSE);
         EnableResizeGroupWithLogic(L"PathsGroup", FALSE);
         EnableResizeGroupWithLogic(L"ImageGroup", FALSE);
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
      else if (LOWORD(wParam) == ID_IMAGE_TYPE && HIWORD(wParam) == CBN_SELCHANGE)
      {
         int selectedIndex = (int)SendMessageW(hImageType, CB_GETCURSEL, 0, 0);
         if (selectedIndex != CB_ERR && selectedIndex < IMAGE_TYPE_COUNT)
         {
            // Copy the label from the struct array into global IMAGE_TYPE buffer
            wcsncpy(g_config.IMAGE_TYPE, g_ImageTypeOptions[selectedIndex].label, sizeof(g_config.IMAGE_TYPE) / sizeof(wchar_t) - 1);
            g_config.IMAGE_TYPE[sizeof(g_config.IMAGE_TYPE) / sizeof(wchar_t) - 1] = L'\0'; // Ensure null-termination
            EnableResizeGroupWithLogic(L"ImageGroup", TRUE);                                // React to change

            // Save the selected image type to the .ini file
            WritePrivateProfileStringW(L"Image", L"IMAGE_TYPE", g_config.IMAGE_TYPE, iniPath);
         }
      }

      // Save checkbox state when any of them is toggled
      for (int i = 0; i < controlCount; ++i)
      {
         if ((HWND)lParam == *controls[i].hCheckbox && HIWORD(wParam) == BN_CLICKED)
         {
            LRESULT checked = SendMessageW(*controls[i].hCheckbox, BM_GETCHECK, 0, 0);
            const wchar_t *value = (checked == BST_CHECKED) ? L"true" : L"false";
            const wchar_t *section = controls[i].configSegment;
            const wchar_t *key = controls[i].configKey;

            WritePrivateProfileStringW(section, key, value, iniPath);

            // Optional direct struct write
            if (controls[i].configField)
            {
               *(controls[i].configField) = (checked == BST_CHECKED);
            }

            // Handle image optimizer + resize state
            if (wcscmp(key, L"hOutputRunImageOptimizer") == 0 ||
                wcscmp(key, L"IMAGE_RESIZE_TO") == 0 ||
                wcscmp(key, L"IMAGE_KEEP_ASPECT_RATIO") == 0)
            {
               // Single clear call to update the group state based on current config
               EnableResizeGroupWithLogic(L"ImageGroup", g_config.runImageOptimizer);

               // Only update layout if the group is actually visible
               if (g_config.runImageOptimizer)
               {
                  AdjustLayout(hwnd);
               }
            }

            break; // processed the toggle
         }
      }

      break;

   case WM_LBUTTONDOWN:
      SetFocus(hListBox);
      break;

   case ID_IMAGE_QUALITY_SLIDER:
   {
      int sliderPos = (int)SendMessageW(hImageQualitySlider, TBM_GETPOS, 0, 0);

      swprintf(g_config.IMAGE_QUALITY, 16, L"%d", sliderPos);
      SetWindowTextW(hImageQualityValue, g_config.IMAGE_QUALITY);

      wchar_t configPath[MAX_PATH];
      GetModuleFileNameW(NULL, configPath, MAX_PATH);

      wchar_t *slashPos = wcsrchr(configPath, L'\\');
      if (slashPos)
         *(slashPos + 1) = L'\0';

      wcscat(configPath, L"config.ini");

      WritePrivateProfileStringW(L"Image", L"IMAGE_QUALITY", g_config.IMAGE_QUALITY, configPath);
   }
   break;

   case WM_HSCROLL:
   {
      if ((HWND)lParam == hImageQualitySlider)
      {
         int sliderPos = (int)SendMessageW(hImageQualitySlider, TBM_GETPOS, 0, 0);
         swprintf(g_config.IMAGE_QUALITY, 16, L"%d", sliderPos);
         SetWindowTextW(hImageQualityValue, g_config.IMAGE_QUALITY);

         wchar_t configPath[MAX_PATH];
         GetModuleFileNameW(NULL, configPath, MAX_PATH);

         wchar_t *slashPos = wcsrchr(configPath, L'\\');
         if (slashPos)
            *(slashPos + 1) = L'\0';

         wcscat(configPath, L"config.ini");

         WritePrivateProfileStringW(L"Image", L"IMAGE_QUALITY", g_config.IMAGE_QUALITY, configPath);
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
      if (hGrayBrush)
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
      else if (lpdis->CtlID == ID_STOP_BUTTON)
      {
         hBmp = hButtonStop;
         bmpW = 70;
         bmpH = 30;
         isHover = isHoverStop;
      }
      else if (lpdis->CtlID == ID_TMP_FOLDER_BROWSE || lpdis->CtlID == ID_OUTPUT_FOLDER_BROWSE ||
               lpdis->CtlID == ID_IMAGEMAGICK_PATH_BROWSE || lpdis->CtlID == ID_SEVEN_ZIP_PATH_BROWSE ||
               lpdis->CtlID == ID_WINRAR_PATH_BROWSE)
      {
         hBmp = hButtonBrowse;
         bmpW = 26;
         bmpH = 26;
         switch (lpdis->CtlID)
         {
         case ID_TMP_FOLDER_BROWSE:
            isHover = isHoverTmp;
            break;
         case ID_OUTPUT_FOLDER_BROWSE:
            isHover = isHoverOutput;
            break;
         case ID_WINRAR_PATH_BROWSE:
            isHover = isHoverWinrar;
            break;
         case ID_SEVEN_ZIP_PATH_BROWSE:
            isHover = isHoverSevenZip;
            break;
         case ID_IMAGEMAGICK_PATH_BROWSE:
            isHover = isHoverImageMagick;
            break;
         }
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
         StretchBlt(hdcScaled, 0, 0, bmpW, bmpH, hdcMem, 0, 0, (lpdis->CtlID == ID_START_BUTTON || lpdis->CtlID == ID_STOP_BUTTON ? 70 : 32),
                    (lpdis->CtlID == ID_START_BUTTON || lpdis->CtlID == ID_STOP_BUTTON ? 30 : 32), SRCCOPY);

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
      HWND transparentControls[] = {hTmpFolderLabel, hOutputFolderLabel, hWinrarLabel, hSevenZipLabel, hImageMagickLabel, hImageQualityLabel};
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
         SendMessage(hwnd, EM_SETSEL, (WPARAM)textLength, (LPARAM)textLength); // Move caret to end
         SendMessage(hwnd, EM_SCROLLCARET, 0, 0);                              // Ensure caret is visible
      }
      break;

   case WM_PROCESSING_DONE:
      ShowWindow(hStartButton, SW_SHOW);
      ShowWindow(hStopButton, SW_HIDE);
      ShowWindow(hTerminalProcessingLabel, SW_HIDE);
      ShowWindow(hTerminalProcessingText, SW_HIDE);
      EnableResizeGroupWithLogic(L"FilesGroup", TRUE);
      EnableResizeGroupWithLogic(L"OutputGroup", TRUE);
      EnableResizeGroupWithLogic(L"PathsGroup", TRUE);
      EnableResizeGroupWithLogic(L"ImageGroup", TRUE);
      AdjustLayout(hwnd);
      break;

   case WM_UPDATE_TERMINAL_TEXT:
   {
      const wchar_t *statusMsg = (const wchar_t *)lParam;
      SetWindowTextW(hTerminalText, statusMsg);
      free((void *)statusMsg);
      return 0;
   }
   break;

   case WM_UPDATE_PROCESSING_TEXT:
   {
      const wchar_t *textMsg = (const wchar_t *)lParam;
      SetWindowTextW(hTerminalProcessingText, textMsg);

      InvalidateRect(hTerminalProcessingText, NULL, TRUE); // force repaint
      UpdateWindow(hTerminalProcessingText);               // redraw immediately

      free((void *)textMsg);
      return 0;
   }
   }
   return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
   (void)hPrevInstance; // Silence warnning - cannot be without it 16bi legacy

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

   // HWND hwnd = CreateWindowW(L"ResizableWindowClass", L"CBRZ Optimizer", WS_OVERLAPPEDWINDOW | WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT,
   //                           500, 400, NULL, NULL, hInstance, NULL);
   HWND hwnd = CreateWindowExW(
       0,                       // dwExStyle
       L"ResizableWindowClass", // lpClassName
       L"CBRZ Optimizer",       // lpWindowName
       WS_OVERLAPPEDWINDOW | WS_THICKFRAME,
       CW_USEDEFAULT, CW_USEDEFAULT,
       500, 400,
       NULL, NULL, hInstance, NULL);

   SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
   SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   if (!IsThemeActive())
   {
      MessageBeep(MB_ICONWARNING); // Play error sound
      MessageBoxCentered(hwnd, L"Visual styles are NOT active!", L"Theme Status", MB_OK | MB_ICONWARNING);
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
      if (!IsDialogMessage(hwnd, &msg))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   return (int)msg.wParam;
}
