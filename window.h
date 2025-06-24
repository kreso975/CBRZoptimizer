#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListBoxProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LabelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void load_config_values(void);

#endif // WINDOW_H