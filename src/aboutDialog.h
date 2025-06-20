#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H

#include <windows.h>

typedef struct
{
   wchar_t CompanyName[128];
   wchar_t FileDescription[128];
   wchar_t FileVersion[64];
   wchar_t InternalName[128];
   wchar_t OriginalFilename[128];
   wchar_t ProductName[128];
   wchar_t ProductVersion[64];
   wchar_t LegalCopyright[256];
} AppVersionInfo;

void ShowAboutWindow(HWND hwndOwner, HINSTANCE hInst);
void GetAppVersionFields(AppVersionInfo *info);

LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
