#include <windows.h>

#define IDC_ABOUT_ICON 101
#define IDC_ABOUT_OK 102

LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static HICON hIcon = NULL;

   switch (msg)
   {
   case WM_CREATE:
      hIcon = (HICON)LoadImageW(NULL, L"icons/comic-64.ico", IMAGE_ICON, 64, 64, LR_LOADFROMFILE | LR_SHARED);
      if (hIcon)
      {
         HWND hIconCtrl = CreateWindowW(
             L"STATIC", NULL,
             WS_CHILD | WS_VISIBLE | SS_ICON,
             20, 20, 64, 64,
             hWnd, (HMENU)IDC_ABOUT_ICON,
             (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
         SendMessageW(hIconCtrl, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
      }

      CreateWindowW(L"STATIC", L"CBRZ Optimizer",
                    WS_CHILD | WS_VISIBLE,
                    140, 20, 200, 20,
                    hWnd, NULL, NULL, NULL);

      HWND hwndAppVersionStatic = CreateWindowW(L"STATIC", L"v0.1.0",
                    WS_CHILD | WS_VISIBLE,
                    170, 50, 200, 20,
                    hWnd, NULL, NULL, NULL);

      HWND hwndCopyRightStatic =CreateWindowW(L"STATIC", L"© 2025 by Krešimir Kokanović", 
                    WS_CHILD | WS_VISIBLE,
                    120, 75, 200, 20,
                    hWnd, NULL, NULL, NULL);

      LOGFONT lf = {0};
      GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
      lf.lfHeight = -12;     // Negative = height in pixels; positive = point size (sort of)
      lf.lfWeight = FW_NORMAL; // FW_NORMAL for regular weight
      // lf.lfItalic = TRUE; // uncomment for italic

      HFONT hAboutFont = CreateFontIndirect(&lf);
      
      SendMessageW(hwndCopyRightStatic, WM_SETFONT, (WPARAM)hAboutFont, TRUE);
      SendMessageW(hwndAppVersionStatic, WM_SETFONT, (WPARAM)hAboutFont, TRUE);


      CreateWindowW(L"BUTTON", L"OK",
                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    110, 125, 80, 25,
                    hWnd, (HMENU)IDC_ABOUT_OK,
                    (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
      break;

   case WM_COMMAND:
      if (LOWORD(wParam) == IDC_ABOUT_OK)
         DestroyWindow(hWnd);
      break;

   case WM_DESTROY:
      if (hIcon)
         DestroyIcon(hIcon);
      return 0;
   }

   return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ShowAboutWindow(HWND hwndOwner, HINSTANCE hInst)
{
   const wchar_t CLASS_NAME[] = L"AboutWindowClass";

   WNDCLASSW wc = {0};
   wc.lpfnWndProc = AboutWndProc;
   wc.hInstance = hInst;
   wc.lpszClassName = CLASS_NAME;
   wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); // System dialog gray


   RegisterClassW(&wc);

   RECT rcOwner;
   GetWindowRect(hwndOwner, &rcOwner);

   int width = 320;
   int height = 190;

   int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - width) / 2;
   int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - height) / 2;

   HWND hWnd = CreateWindowExW(
       WS_EX_DLGMODALFRAME,
       CLASS_NAME, L"About",
       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
       x, y, width, height,
       hwndOwner, NULL, hInst, NULL);

   ShowWindow(hWnd, SW_SHOW);
   UpdateWindow(hWnd);

   // Modal-style message loop
   MSG msg;
   while (IsWindow(hWnd) && GetMessageW(&msg, NULL, 0, 0))
   {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
   }
}
