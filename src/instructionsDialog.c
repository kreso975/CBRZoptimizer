#include <windows.h>
#include <wchar.h>
#include <richedit.h>
#include <shellapi.h>
#include "instructionsDialog.h"
#include "resource.h"

LRESULT CALLBACK InstructionsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static HFONT hFontEmoji = NULL;
   static HFONT hFontTitle = NULL;
   static HWND hRichEdit = NULL;

   switch (msg)
   {
   case WM_CREATE:
   {
      LoadLibraryW(L"Msftedit.dll");

      // Load and show icon
      HICON hIcon = (HICON)LoadImageW(
          GetModuleHandleW(NULL),
          MAKEINTRESOURCEW(IDI_ICON1),
          IMAGE_ICON,
          32, 32,
          LR_DEFAULTCOLOR | LR_SHARED);

      if (hIcon)
      {
         HWND hIconCtrl = CreateWindowW(
             L"STATIC", NULL,
             WS_CHILD | WS_VISIBLE | SS_ICON,
             20, 20, 32, 32,
             hWnd, NULL,
             NULL, NULL);

         SendMessageW(hIconCtrl, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
      }

      // Create title label beside icon
      hFontTitle = CreateFontW(
          -16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

      HWND hTitleLabel = CreateWindowW(
          L"STATIC", L"CBRZoptimizer Instructions",
          WS_CHILD | WS_VISIBLE,
          70, 28, 400, 24,
          hWnd, NULL, NULL, NULL);

      SendMessageW(hTitleLabel, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

      // Create RichEdit control below header
      hRichEdit = CreateWindowExW(
          0, L"RICHEDIT50W", NULL,
          WS_CHILD | WS_VISIBLE | WS_VSCROLL |
              ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
          20, 60, 540, 360,
          hWnd, (HMENU)IDC_INSTRUCTIONS_TEXT,
          NULL, NULL);

      SendMessageW(hRichEdit, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
      SendMessageW(hRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageW(hRichEdit, EM_SETEVENTMASK, 0, ENM_LINK);

      hFontEmoji = CreateFontW(
          -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Emoji");

      SendMessageW(hRichEdit, WM_SETFONT, (WPARAM)hFontEmoji, TRUE);
  // ⚙️ ✨ ℹ️✔️ ❗ ❓✏️☕ ☀️ ☁️ ☂️ ☃️ ☔️ ⚡️ ❄️ ☑️ ☢️ ☣️ ☠️ ☤ ☮️ ☯️ ☸️ ☹️ ☺️ ⚓ ⚡⚪ ⚫ ⚽ ⚾ ⛄ ⛅ ⛔ ⛪ ⛲ ⛳ ⛵ ⛹ ⛺ ⛽ ✅ ✉ ✊ ✋ ✌ ✍ ✨ ✪ 
      const wchar_t *helpText =
          L"⚙️ Built-in Features:\r\n"
          L"- You can extract .cbr and .cbz comic archives directly.\r\n"
          L"- Images inside archives can be optimized using the built-in STB library.\r\n"
          L"- After optimization, images are repacked into a clean .cbz archive.\r\n"
          L"- Drag and drop is supported — just drop files or folders into the app window.\r\n\r\n"
          L"⚡️ Optional External Tools:\r\n"
          L"- WinRAR: https://www.win-rar.com — for handling .rar and .cbr files.\r\n"
          L"- 7-Zip: https://www.7-zip.org — alternative archive extraction.\r\n"
          L"- MuPDF: https://mupdf.com — required for converting to and from PDF.\r\n"
          L"- ImageMagick: https://imagemagick.org — advanced image optimization.\r\n\r\n"
          L"✅ Supported Formats:\r\n"
          L"- The app works with .cbz, .cbr, .zip, .rar, and .pdf files.\r\n\r\n"
          L"✨ Features:\r\n"
          L"- Convert comic archives to PDF (requires MuPDF).\r\n"
          L"- Convert PDF documents back into .cbz or .cbr comic archives.\r\n"
          L"- Optimize image files using either STB or ImageMagick.\r\n\r\n"
          L"✍ How to Use:\r\n"
          L"1. Select one or more input files or folders.\r\n"
          L"2. Choose your desired output format: CBZ, CBR, or PDF.\r\n"
          L"3. Click the Optimize button and let the tool do the rest.\r\n\r\n"
          L"ℹ️ For more info and updates: https://github.com/kreso975/CBRZoptimizer";

      SetWindowTextW(hRichEdit, helpText);

      // Apply font to entire body
      CHARFORMAT2W cfBody = {0};
      cfBody.cbSize = sizeof(cfBody);
      cfBody.dwMask = CFM_SIZE | CFM_FACE;
      cfBody.yHeight = 200; // 10pt
      wcscpy(cfBody.szFaceName, L"Segoe UI Emoji");

      CHARRANGE bodyRange = {0, -1};
      SendMessageW(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&bodyRange);
      SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfBody);

      // Clear selection and scroll to top
      CHARRANGE clearRange = {-1, -1};
      SendMessageW(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&clearRange);
      SendMessageW(hRichEdit, EM_SCROLL, SB_TOP, 0);

      CreateWindowW(L"BUTTON", L"OK",
                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    260, 440, 80, 25,
                    hWnd, (HMENU)IDC_INSTRUCTIONS_OK,
                    NULL, NULL);
      break;
   }

   case WM_COMMAND:
      if (LOWORD(wParam) == IDC_INSTRUCTIONS_OK)
         DestroyWindow(hWnd);
      break;

   case WM_NOTIFY:
   {
      NMHDR *nmhdr = (NMHDR *)lParam;
      if (nmhdr->idFrom == IDC_INSTRUCTIONS_TEXT && nmhdr->code == EN_LINK)
      {
         ENLINK *enLink = (ENLINK *)lParam;
         if (enLink->msg == WM_LBUTTONDOWN)
         {
            wchar_t url[512] = {0};
            TEXTRANGEW tr = {{enLink->chrg.cpMin, enLink->chrg.cpMax}, url};
            SendMessageW(hRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
            ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
         }
      }
      break;
   }

   case WM_DESTROY:
      if (hFontEmoji)
         DeleteObject(hFontEmoji);
      if (hFontTitle)
         DeleteObject(hFontTitle);
      return 0;
   }

   return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ShowInstructionsWindow(HWND hwndOwner, HINSTANCE hInst)
{
   const wchar_t CLASS_NAME[] = L"InstructionsWindowClass";

   WNDCLASSW wc = {0};
   wc.lpfnWndProc = InstructionsWndProc;
   wc.hInstance = hInst;
   wc.lpszClassName = CLASS_NAME;
   wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

   RegisterClassW(&wc);

   RECT rcOwner;
   GetWindowRect(hwndOwner, &rcOwner);

   int width = 570;
   int height = 500;

   int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - width) / 2;
   int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - height) / 2;

   EnableWindow(hwndOwner, FALSE);

   HWND hWnd = CreateWindowExW(
       WS_EX_DLGMODALFRAME,
       CLASS_NAME, L"Instructions",
       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
       x, y, width, height,
       hwndOwner, NULL, hInst, NULL);

   ShowWindow(hWnd, SW_SHOW);
   UpdateWindow(hWnd);

   MSG msg;
   while (IsWindow(hWnd) && GetMessageW(&msg, NULL, 0, 0))
   {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
   }

   EnableWindow(hwndOwner, TRUE);
   SetActiveWindow(hwndOwner);
}