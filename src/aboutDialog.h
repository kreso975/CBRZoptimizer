#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H

#include <windows.h>
#include "versioninfo.h"  // Include the shared version info header

void ShowAboutWindow(HWND hwndOwner, HINSTANCE hInst);

LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
