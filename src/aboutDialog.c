#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <commctrl.h> // Required for SysLink

#include "aboutDialog.h"
#include "resource.h"

void GetAppVersionFields(AppVersionInfo *info)
{
   memset(info, 0, sizeof(*info));

   wchar_t path[MAX_PATH];
   GetModuleFileNameW(NULL, path, MAX_PATH);

   DWORD dummy = 0;
   DWORD size = GetFileVersionInfoSizeW(path, &dummy);
   if (size == 0)
      return;

   BYTE *data = (BYTE *)malloc(size);
   if (!GetFileVersionInfoW(path, 0, size, data))
   {
      free(data);
      return;
   }

   struct LANGANDCODEPAGE
   {
      WORD wLanguage;
      WORD wCodePage;
   } *lpTranslate;
   UINT cbTranslate = 0;
   if (!VerQueryValueW(data, L"\\VarFileInfo\\Translation", (LPVOID *)&lpTranslate, &cbTranslate))
   {
      free(data);
      return;
   }

   wchar_t subBlock[64];
   wchar_t *value = NULL;
   UINT len = 0;
   WORD lang = lpTranslate[0].wLanguage;
   WORD code = lpTranslate[0].wCodePage;

#define FETCH(field, target)                                                  \
   swprintf(subBlock, 64, L"\\StringFileInfo\\%04x%04x\\" field, lang, code); \
   if (VerQueryValueW(data, subBlock, (LPVOID *)&value, &len) && value)       \
   wcsncpy(target, value, len)

   FETCH(L"CompanyName", info->CompanyName);
   FETCH(L"FileDescription", info->FileDescription);
   FETCH(L"FileVersion", info->FileVersion);
   FETCH(L"InternalName", info->InternalName);
   FETCH(L"OriginalFilename", info->OriginalFilename);
   FETCH(L"ProductName", info->ProductName);
   FETCH(L"ProductVersion", info->ProductVersion);
   FETCH(L"LegalCopyright", info->LegalCopyright);

#undef FETCH

   free(data);
}

LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static HICON hIcon = NULL;

   switch (msg)
   {
   case WM_CREATE:
   {
      hIcon = (HICON)LoadImageW( GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON,
         64, 64, LR_DEFAULTCOLOR | LR_SHARED);

      if (hIcon)
      {
         HWND hIconCtrl = CreateWindowW( L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_ICON, 20, 20, 64, 64,
            hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

         SendMessageW(hIconCtrl, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
      }

      AppVersionInfo info;
      GetAppVersionFields(&info);

      CreateWindowW(L"STATIC", info.ProductName, WS_CHILD | WS_VISIBLE, 140, 20, 200, 20, hWnd, NULL, NULL, NULL);
      HWND hwndAppVersionStatic = CreateWindowW(L"STATIC", info.ProductVersion, WS_CHILD | WS_VISIBLE, 170, 48, 200, 20, hWnd, NULL, NULL, NULL);
      HWND hwndCopyRightStatic = CreateWindowW(L"STATIC", L"© 2025 by Krešimir Kokanović", WS_CHILD | WS_VISIBLE, 120, 70, 200, 20, hWnd, NULL, NULL, NULL);

      HWND hwndIconsLink = CreateWindowExW( 0, WC_LINK, L"Icons by <A HREF=\"https://icons8.com\">Icons8</A>",
          WS_CHILD | WS_VISIBLE | WS_TABSTOP, 135, 95, 200, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

      LOGFONT lf = {0};
      GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
      lf.lfHeight = -12;
      lf.lfWeight = FW_NORMAL;

      HFONT hAboutFont = CreateFontIndirect(&lf);

      SendMessageW(hwndCopyRightStatic, WM_SETFONT, (WPARAM)hAboutFont, TRUE);
      SendMessageW(hwndAppVersionStatic, WM_SETFONT, (WPARAM)hAboutFont, TRUE);
      SendMessageW(hwndIconsLink, WM_SETFONT, (WPARAM)hAboutFont, TRUE);

      CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    110, 125, 80, 25, hWnd, (HMENU)IDC_ABOUT_OK,
                    (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
      break;
   }

   case WM_COMMAND:
      if (LOWORD(wParam) == IDC_ABOUT_OK)
         DestroyWindow(hWnd);
      break;

   case WM_NOTIFY:
   {
      NMHDR *nmh = (NMHDR *)lParam;
      if (nmh->code == NM_CLICK || nmh->code == NM_RETURN)
      {
         NMLINK *link = (NMLINK *)lParam;
         ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
      }
      break;
   }

   case WM_DESTROY:
      if (hIcon)
         DestroyIcon(hIcon);
      return 0;
   }

   return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ShowAboutWindow(HWND hwndOwner, HINSTANCE hInst)
{
   INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LINK_CLASS };
   InitCommonControlsEx(&icex);

   const wchar_t CLASS_NAME[] = L"AboutWindowClass";

   WNDCLASSW wc = {0};
   wc.lpfnWndProc = AboutWndProc;
   wc.hInstance = hInst;
   wc.lpszClassName = CLASS_NAME;
   wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

   RegisterClassW(&wc);

   RECT rcOwner;
   GetWindowRect(hwndOwner, &rcOwner);

   int width = 320;
   int height = 190;

   int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - width) / 2;
   int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - height) / 2;

   EnableWindow(hwndOwner, FALSE);

   HWND hWnd = CreateWindowExW(
       WS_EX_DLGMODALFRAME,
       CLASS_NAME, L"About",
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