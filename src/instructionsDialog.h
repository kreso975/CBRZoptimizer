#ifndef INSTRUCTIONSDIALOG_H
#define INSTRUCTIONSDIALOG_H

#include <windows.h>

LRESULT CALLBACK InstructionsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ShowInstructionsWindow(HWND hwndOwner, HINSTANCE hInst);

#endif // INSTRUCTIONSDIALOG_H