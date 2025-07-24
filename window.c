#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h> // For slider control, etc.
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
#include "instructionsDialog.h"
#include "debug.h"

#include <uxtheme.h>

HINSTANCE g_hInstance;

HFONT hBoldFont, hFontLabel, hFontInput, hFontEmoji;
HBITMAP hButtonPlus, hButtonMinus, hButtonAddFolder, hButtonBrowse, hButtonStart, hButtonStop, hButtonOpenInFolder;
// Main controls
HWND hListBox, hTooltip, hStartButton, hStopButton, hAddButton, hRemoveButton, hAddFolderButton, hLabelNumberOfFiles, hNumberOfFiles, hSettingsWnd;
HWND hOpenInTmpFolderButton, hOpenInOutputFolderButton;
// Paths
HWND hTmpFolder, hOutputFolder, hWinrarPath, hSevenZipPath, hImageMagickPath, hMuToolPath, hImageResize;
HWND hTmpBrowse, hOutputBrowse, hWinrarBrowse, hSevenZipBrowse, hImageMagickBrowse, hMuToolBrowse;
// Groups
HWND hFilesGroup, hTerminalGroup, hSettingsGroup, hImageSettingsGroup, hOutputGroup;
// Terminal
HWND hTerminalProcessingLabel, hTerminalProcessingText, hTerminalText;
// Labels
HWND hTmpFolderLabel, hOutputFolderLabel, hWinrarLabel, hSevenZipLabel, hMuToolLabel;
HWND hImageTypeLabel, hImageAllowUpscalingLabel, hImageResizeToLabel, hImageMagickLabel;
HWND hImageQualityLabel, hImageQualityValue, hImageSizeWidthLabel, hImageSizeHeightLabel, hImageKeepAspectRatioLabel;
// Image settings
HWND hImageType, hImageAllowUpscaling, hImageResizeTo, hImageQualitySlider, hImageSizeWidth, hImageSizeHeight, hImageKeepAspectRatio;
// Output options
HWND hOutputKeepExtractedLabel, hOutputKeepExtracted, hOutputExtractCover, hOutputExtractCoverLabel, hOutputRunExtractLabel, hOutputRunExtract;
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
    .MUTOOL_PATH = L"",

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
    .keepExtracted = TRUE,
    .extractCover = TRUE};

extern LabelCheckboxPair controls[];
extern const size_t controlCount;

GUIHandleEntry groupElements[] = {
    {L"ListBox", L"FilesGroup", &hListBox},
    {L"AddButton", L"FilesGroup", &hAddButton},
    {L"RemoveButton", L"FilesGroup", &hRemoveButton},
    {L"AddFolderButton", L"FilesGroup", &hAddFolderButton},

    {L"TmpFolder", L"PathsGroup", &hTmpFolder},
    {L"TmpFolder Label", L"PathsGroup", &hTmpFolderLabel},
    {L"TmpFolder Browse", L"PathsGroup", &hTmpBrowse},
    {L"OutputFolder", L"PathsGroup", &hOutputFolder},
    {L"Open Tmp Folder", L"PathsGroup", &hOpenInTmpFolderButton},
    {L"Open Output Folder", L"PathsGroup", &hOpenInOutputFolderButton},
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
    {L"Mutoool Path", L"PathsGroup", &hMuToolPath},
    {L"Mutool Label", L"PathsGroup", &hMuToolLabel},
    {L"Mutool Browse", L"PathsGroup", &hMuToolBrowse},

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
    {L"hOutputRunCompressorLabel", L"OutputGroup", &hOutputRunCompressorLabel},
    {L"hOutputExtractCover", L"OutputGroup", &hOutputExtractCover},
    {L"hOutputExtractCoverLabel", L"OutputGroup", &hOutputExtractCoverLabel}};

int groupElementsCount = sizeof(groupElements) / sizeof(groupElements[0]);

const ImageTypeEntry g_ImageTypeOptions[] = {
    {L"Portrait", IMAGE_TYPE_PORTRAIT},
    {L"Landscape", IMAGE_TYPE_LANDSCAPE}};

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

         for (size_t i = 0; i < controlCount; ++i)
         {
            if (hCheckbox == *controls[i].hCheckbox)
            {
               const wchar_t *section = controls[i].configSegment;
               const wchar_t *key = controls[i].configKey;
               const wchar_t *value = (newState == BST_CHECKED) ? L"true" : L"false";

               WritePrivateProfileStringW(section, key, value, iniPath);

               // Update g_config if needed
               if (wcscmp(key, L"OUTPUT_RUN_IMAGE_OPTIMIZER") == 0)
                  g_config.runImageOptimizer = (newState == BST_CHECKED);
               else if (wcscmp(key, L"OUTPUT_RUN_COMPRESSOR") == 0)
                  g_config.runCompressor = (newState == BST_CHECKED);
               else if (wcscmp(key, L"OUTPUT_KEEP_EXTRACTED") == 0)
                  g_config.keepExtracted = (newState == BST_CHECKED);
               else if (wcscmp(key, L"OUTPUT_EXTRACT_COVER") == 0)
                  g_config.extractCover = (newState == BST_CHECKED);
               else if (wcscmp(key, L"IMAGE_RESIZE_TO") == 0)
                  g_config.resizeTo = (newState == BST_CHECKED);
               else if (wcscmp(key, L"IMAGE_KEEP_ASPECT_RATIO") == 0)
                  g_config.keepAspectRatio = (newState == BST_CHECKED);
               else if (wcscmp(key, L"IMAGE_ALLOW_UPSCALING") == 0)
                  g_config.allowUpscaling = (newState == BST_CHECKED);
               break;
            }
         }

         for (size_t i = 0; i < controlCount; ++i)
         {
            if (hwnd == *controls[i].hLabel && controls[i].triggersGroupLogic)
            {
               BOOL shouldEnable = g_config.runImageOptimizer;

               EnableResizeGroupWithLogic(L"ImageGroup", shouldEnable);

               if (shouldEnable)
                  AdjustLayout(GetParent(hwnd)); // ✅ Only GetParent(hwnd) works for label

               break;
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

LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static TRACKMOUSEEVENT tme;
   int rawId = GetDlgCtrlID(hwnd);
   if (rawId == -1)
      return DefWindowProc(hwnd, msg, wParam, lParam);

   UINT ctlId = (UINT)rawId;

   for (size_t i = 0; i < buttonsCount; ++i)
   {
      if (buttons[i].id == ctlId)
      {
         BOOL *hoverFlag = &buttons[i].isHover;

         switch (msg)
         {
         case WM_MOUSEMOVE:
            if (!*hoverFlag)
            {
               *hoverFlag = TRUE;
               tme = (TRACKMOUSEEVENT){sizeof(tme), TME_LEAVE, hwnd, 0};
               TrackMouseEvent(&tme);
               InvalidateRect(hwnd, NULL, TRUE);
            }
            break;

         case WM_MOUSELEAVE:
            if (*hoverFlag)
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
   }

   return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, msg, wParam, lParam);
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

      hFontInput = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

      hFontLabel = CreateFontW(-13, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

      hFontEmoji = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Emoji");

      static HBRUSH hGrayBrush = NULL;
      hGrayBrush = CreateSolidBrush(RGB(192, 192, 192));

      int defaultWidth = 960;  // 800 * 1.2 (20% increase)
      int defaultHeight = 625; // Keep height unchanged unless needed
      MoveWindow(hwnd, 100, 100, defaultWidth, defaultHeight, TRUE);

      HMENU hMenu = CreateMenu();
      HMENU menuFile = CreatePopupMenu();
      HMENU helpMenu = CreatePopupMenu(); // ✅ Single Help menu

      AppendMenuW(menuFile, MF_STRING, ID_FILE_EXIT, L"Exit");

      AppendMenuW(helpMenu, MF_STRING, ID_INSTRUCTIONS_HELP, L"Instructions");
      AppendMenuW(helpMenu, MF_STRING, ID_CHECK_FOR_UPDATE, L"Check for Update");
      AppendMenuW(helpMenu, MF_STRING, ID_HELP_ABOUT, L"About");

      AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)menuFile, L"File");
      AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)helpMenu, L"Help"); // ✅ Add combined Help menu

      // Right-align "Help"
      MENUITEMINFO mii = {0};
      mii.cbSize = sizeof(MENUITEMINFO);
      mii.fMask = MIIM_FTYPE;
      mii.fType = MFT_RIGHTJUSTIFY;
      SetMenuItemInfo(hMenu, (UINT)(GetMenuItemCount(hMenu) - 1), TRUE, &mii);

      SetMenu(hwnd, hMenu);

      hTooltip = CreateWindowExW(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT, hwnd, NULL, GetModuleHandleW(NULL), NULL);

      TOOLINFO ti = {0};
      ti.cbSize = sizeof(TOOLINFO);
      ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
      ti.hwnd = hwnd;

      /*
       * Iterates over an array of ButtonSpec structures to create and initialize custom owner-drawn buttons.
       * 
       * For each button:
       *   - Determines the window style, including visibility.
       *   - Creates the button window and stores its handle.
       *   - Checks if the bitmap image for the button has already been loaded by a previous button with the same bmpId.
       *     - If already loaded, reuses the existing bitmap handle.
       *     - Otherwise, loads the bitmap resource and handles loading errors with a message box and beep.
       *   - Sets the loaded bitmap as the button image.
       *   - Subclasses the button window procedure to use a custom ButtonProc.
       *   - Adds a tooltip for the button using the provided tooltip text.
       *
       * Parameters:
       *   - buttons: Array of ButtonSpec structures describing each button.
       *   - buttonsCount: Number of buttons in the array.
       *   - hwnd: Handle to the parent window.
       *   - hTooltip: Handle to the tooltip control.
       *   - ti: TOOLINFO structure used for tooltip configuration.
       *
       * Assumptions:
       *   - Each ButtonSpec contains pointers to window and bitmap handles, position, size, resource IDs, and tooltip text.
       *   - ButtonProc is a valid window procedure for custom button handling.
       *   - MessageBoxCentered is a helper function to display centered message boxes.
       */
      for (size_t i = 0; i < buttonsCount; ++i)
      {
         ButtonSpec *b = &buttons[i];

         DWORD style = WS_CHILD | BS_OWNERDRAW | WS_TABSTOP;
         if (b->visible)
            style |= WS_VISIBLE;

         *b->handle = CreateWindowW( L"BUTTON", L"", style, b->x, b->y, b->imageW, b->imageH, hwnd, (HMENU)(INT_PTR)b->id, NULL, NULL);

         // Check if bitmap already loaded by previous button
         BOOL alreadyLoaded = FALSE;
         for (size_t j = 0; j < i; ++j)
         {
            if (buttons[j].bmpId == b->bmpId)
            {
                  *b->bmpHandle = *buttons[j].bmpHandle;
                  alreadyLoaded = TRUE;
                  break;
            }
         }

         // Load only if not already loaded
         if (!alreadyLoaded)
         {
            *b->bmpHandle = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(b->bmpId));
            if (!*b->bmpHandle)
            {
                  MessageBeep(MB_ICONERROR);
                  MessageBoxCentered(hwnd, L"Failed to load button image!", L"Error", MB_OK | MB_ICONERROR);
            }
         }

         if (*b->bmpHandle)
         {
            SendMessageW(*b->handle, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)*b->bmpHandle);
         }

         SetWindowLongPtr(*b->handle, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(*b->handle, GWLP_WNDPROC));
         SetWindowLongPtr(*b->handle, GWLP_WNDPROC, (LONG_PTR)ButtonProc);

         ti.uId = (UINT_PTR)*b->handle;
         ti.lpszText = (LPWSTR)b->tooltip;
         SendMessageW(hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
      }

      // -----------------------------------------------------------------------------------------------

      // Optional: Set tooltip behavior
      SendMessageW(hTooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 100);
      SendMessageW(hTooltip, TTM_SETMAXTIPWIDTH, 0, 300);

      // **Files Group (Left)**
      hFilesGroup = CreateWindowW(L"BUTTON", L"Files", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 10, 300, 200, hwnd, NULL, NULL, NULL);

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
         SendMessageW(hTerminalText, WM_SETFONT, (WPARAM)hFontEmoji, TRUE);

      // **Settings Group (Right)**
      hSettingsGroup = CreateWindowW(L"BUTTON", L"Settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                     320, 10, 480, 210, hwnd, NULL, NULL, NULL);

      for (size_t i = 0; i < inputsCount; ++i)
      {
         // Create label
         *inputs[i].hLabel = CreateWindowW( L"STATIC", inputs[i].labelText, WS_CHILD | WS_VISIBLE,
             20, inputs[i].y, inputs[i].labelWidth, 20, hwnd, NULL, NULL, NULL);

         // Create edit field
         *inputs[i].hEdit = CreateWindowW( L"EDIT", inputs[i].defaultText, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP,
             120, inputs[i].y, inputs[i].editWidth, 20, hwnd, NULL, NULL, NULL);

         // Apply fonts
         SendMessageW(*inputs[i].hLabel, WM_SETFONT, (WPARAM)hFontLabel, TRUE);
         SendMessageW(*inputs[i].hEdit, WM_SETFONT, (WPARAM)hFontInput, TRUE);
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

      for (size_t i = 0; i < controlCount; ++i)
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
      MoveWindow(hAddFolderButton, rect.right - 420, 30, 32, 32, TRUE);
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
      MoveWindow(hSettingsGroup, rect.right - 360, 10, 350, 223, TRUE);
      MoveWindow(hImageSettingsGroup, rect.right - 360, 243, 350, 180, TRUE);
      MoveWindow(hOutputGroup, rect.right - 360, 433, 350, 120, TRUE);

      // **Settings Fields and Labels**
      MoveWindow(hTmpFolderLabel, rect.right - 350, 40, 100, 20, TRUE);
      MoveWindow(hOpenInTmpFolderButton, rect.right - 260, 42, 16, 16, TRUE);
      MoveWindow(hTmpFolder, rect.right - 240, 40, 180, 20, TRUE);
      MoveWindow(hTmpBrowse, rect.right - 50, 32, 30, 30, TRUE);

      MoveWindow(hOutputFolderLabel, rect.right - 350, 72, 100, 20, TRUE);
      MoveWindow(hOpenInOutputFolderButton, rect.right - 260, 74, 16, 16, TRUE);
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

      MoveWindow(hMuToolLabel, rect.right - 350, 204, 100, 20, TRUE);
      MoveWindow(hMuToolPath, rect.right - 240, 204, 180, 20, TRUE);
      MoveWindow(hMuToolBrowse, rect.right - 50, 197, 30, 30, TRUE);

      // **Image Settings Fields**
      MoveWindow(hImageQualityLabel, rect.right - 350, 273, 100, 20, TRUE);
      MoveWindow(hImageQualityValue, rect.right - 250, 273, 100, 20, TRUE);
      MoveWindow(hImageQualitySlider, rect.right - 350, 298, 330, 30, TRUE);
      MoveWindow(hImageTypeLabel, rect.right - 350, 343, 90, 20, TRUE);
      MoveWindow(hImageType, rect.right - 270, 338, 120, 20, TRUE);

      MoveWindow(hImageResizeTo, rect.right - 350, 373, 20, 20, TRUE);
      MoveWindow(hImageResizeToLabel, rect.right - 330, 375, 80, 20, TRUE);

      MoveWindow(hImageSizeWidthLabel, rect.right - 245, 373, 35, 20, TRUE);
      MoveWindow(hImageSizeWidth, rect.right - 205, 373, 50, 20, TRUE);
      MoveWindow(hImageSizeHeightLabel, rect.right - 130, 373, 45, 20, TRUE);
      MoveWindow(hImageSizeHeight, rect.right - 85, 373, 50, 20, TRUE);

      MoveWindow(hImageKeepAspectRatio, rect.right - 350, 398, 20, 20, TRUE);
      MoveWindow(hImageKeepAspectRatioLabel, rect.right - 330, 400, 100, 20, TRUE);

      MoveWindow(hImageAllowUpscaling, rect.right - 150, 398, 20, 20, TRUE);
      MoveWindow(hImageAllowUpscalingLabel, rect.right - 130, 400, 100, 20, TRUE);

      MoveWindow(hOutputTypeLabel, rect.right - 350, 463, 80, 20, TRUE);
      MoveWindow(hOutputType, rect.right - 280, 458, 120, 20, TRUE);
      MoveWindow(hOutputRunImageOptimizer, rect.right - 350, 493, 20, 20, TRUE);
      MoveWindow(hOutputRunImageOptimizerLabel, rect.right - 330, 495, 140, 20, TRUE);
      MoveWindow(hOutputRunCompressor, rect.right - 150, 493, 20, 20, TRUE);
      MoveWindow(hOutputRunCompressorLabel, rect.right - 130, 495, 110, 20, TRUE);
      MoveWindow(hOutputKeepExtracted, rect.right - 350, 513, 20, 20, TRUE);
      MoveWindow(hOutputKeepExtractedLabel, rect.right - 330, 515, 150, 20, TRUE);
      MoveWindow(hOutputExtractCover, rect.right - 150, 513, 20, 20, TRUE);
      MoveWindow(hOutputExtractCoverLabel, rect.right - 130, 515, 110, 20, TRUE);

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
         update_output_type_dropdown(); // clean, direct
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
      else if (LOWORD(wParam) == ID_MUTOOL_PATH_BROWSE)
      {
         BrowseFile(hwnd, g_config.MUTOOL_PATH);
         WritePrivateProfileStringW(L"Paths", L"MUTOOL_PATH", g_config.MUTOOL_PATH, iniPath);
         SetWindowTextW(hMuToolPath, g_config.MUTOOL_PATH);
         update_output_type_dropdown(); // clean, direct
      }
      else if (LOWORD(wParam) == ID_OPEN_IN_TMP_FOLDER_BUTTON)
      {
         // Open File Explorer at TMP_FOLDER
         ShellExecuteW(NULL, L"open", g_config.TMP_FOLDER, NULL, NULL, SW_SHOWNORMAL);
      }
      else if (LOWORD(wParam) == ID_OPEN_IN_OUTPUT_FOLDER_BUTTON)
      {
         // Open File Explorer at OUTPUT_FOLDER
         ShellExecuteW(NULL, L"open", g_config.OUTPUT_FOLDER, NULL, NULL, SW_SHOWNORMAL);
      }
      else if (HIWORD(wParam) == EN_KILLFOCUS)
      {
         wchar_t buffer[MAX_PATH];
         GetWindowTextW((HWND)lParam, buffer, MAX_PATH);

         for (size_t i = 0; i < inputsCount; ++i)
         {
            if (*(inputs[i].hEdit) == (HWND)lParam)
            {
               ValidateAndSaveInput(hwnd, (HWND)lParam, iniPath);
               break;
            }
         }

         if ((HWND)lParam == hWinrarPath)
            update_output_type_dropdown();
         else if ((HWND)lParam == hMuToolPath)
            update_output_type_dropdown();
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
      else if (LOWORD(wParam) == ID_ADD_FOLDER_BUTTON)
      {
         wchar_t folderPath[MAX_PATH];
         BrowseFolder(hwnd, folderPath);
         AddUniqueToListBox(hwnd, hListBox, folderPath);
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
      else if (LOWORD(wParam) == ID_INSTRUCTIONS_HELP)
      {
         ShowInstructionsWindow(hwnd, g_hInstance);
      }
      else if (LOWORD(wParam) == ID_CHECK_FOR_UPDATE)
      {
         CheckForUpdate(hwnd, FALSE);
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

      for (size_t i = 0; i < controlCount; ++i)
      {
         if ((HWND)lParam == *controls[i].hCheckbox && HIWORD(wParam) == BN_CLICKED)
         {
            LRESULT checked = SendMessageW(*controls[i].hCheckbox, BM_GETCHECK, 0, 0);
            const wchar_t *value = (checked == BST_CHECKED) ? L"true" : L"false";

            WritePrivateProfileStringW(controls[i].configSegment, controls[i].configKey, value, iniPath);

            if (controls[i].configField)
               *(controls[i].configField) = (checked == BST_CHECKED);

            // ✅ Simplified layout logic using triggersGroupLogic flag
            if (controls[i].triggersGroupLogic)
            {
               EnableResizeGroupWithLogic(L"ImageGroup", TRUE);

               if (g_config.runImageOptimizer || g_config.extractCover || g_config.runCompressor)
                  AdjustLayout(hwnd);
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
   {
      HDROP hDrop = (HDROP)wParam;

      // Get cursor position in screen coordinates
      POINT pt;
      DragQueryPoint(hDrop, &pt);
      ClientToScreen(hwnd, &pt);

      // Get the window under the cursor
      HWND hTarget = WindowFromPoint(pt);

      // Check if it's one of your edit controls
      if (hTarget == hTmpFolder || hTarget == hOutputFolder)
      {
         wchar_t filePath[MAX_PATH];
         if (DragQueryFileW(hDrop, 0, filePath, MAX_PATH) > 0)
         {
            DWORD attr = GetFileAttributesW(filePath);
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
            {
               SetWindowTextW(hTarget, filePath);
            }
         }
      }
      else
      {
         // Fallback to listbox handling
         ProcessDroppedFiles(hwnd, hListBox, hDrop);
      }

      DragFinish(hDrop);
      return 0;
   }

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
      int bmpW = 0, bmpH = 0;

      // First: try to match a ButtonSpec
      for (size_t i = 0; i < buttonsCount; ++i)
      {
         if (lpdis->CtlID == buttons[i].id)
         {
            hBmp = *buttons[i].bmpHandle;
            bmpW = buttons[i].imageW;
            bmpH = buttons[i].imageH;
            isHover = buttons[i].isHover; // 👈 Use hover state directly from struct
            break;
         }
      }

      // If we have a valid bitmap, draw it
      if (hBmp)
      {
         HDC hdcMem = CreateCompatibleDC(lpdis->hDC);
         HGDIOBJ oldBmp = SelectObject(hdcMem, hBmp);

         // Apply hover effect
         int drawW = bmpW - (isHover ? 2 : 0);
         int drawH = bmpH - (isHover ? 2 : 0);

         // Center the bitmap
         int btnW = lpdis->rcItem.right - lpdis->rcItem.left;
         int btnH = lpdis->rcItem.bottom - lpdis->rcItem.top;
         int x = (btnW - drawW) / 2;
         int y = (btnH - drawH) / 2;

         // Create scaled bitmap
         HDC hdcScaled = CreateCompatibleDC(lpdis->hDC);
         HBITMAP hScaledBmp = CreateCompatibleBitmap(lpdis->hDC, drawW, drawH);
         HGDIOBJ oldScaledBmp = SelectObject(hdcScaled, hScaledBmp);

         StretchBlt(hdcScaled, 0, 0, drawW, drawH, hdcMem, 0, 0, bmpW, bmpH, SRCCOPY);

         // Transparent blit
         TransparentBlt(lpdis->hDC, x, y, drawW, drawH, hdcScaled, 0, 0, drawW, drawH, RGB(255, 255, 255));

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

   // 🔒 Single-instance protection
   HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Global\\CBRZoptimizerMutex");
   if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
   {
      HWND existing = FindWindowW(L"ResizableWindowClass", L"CBRZ Optimizer");
      if (existing)
      {
         ShowWindow(existing, SW_RESTORE); // Restore if minimized
         SetForegroundWindow(existing);    // Bring to front
      }
      // No message box needed — we’re handing control to the existing instance
      return 0;
   }

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

   HWND hwnd = CreateWindowExW(
       0,                       // dwExStyle
       L"ResizableWindowClass", // lpClassName
       L"CBRZ Optimizer",       // lpWindowName
       WS_OVERLAPPEDWINDOW | WS_THICKFRAME,
       CW_USEDEFAULT, CW_USEDEFAULT,
       500, 440,
       NULL, NULL, hInstance, NULL);

   SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
   SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   // This should have .ini settings
   CheckForUpdate(hwnd, TRUE);

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

   // 🧹 Clean up mutex
   if (hMutex)
   {
      ReleaseMutex(hMutex);
      CloseHandle(hMutex);
   }

   return (int)msg.wParam;
}
