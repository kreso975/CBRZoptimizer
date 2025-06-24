#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void load_config_values(void);

#endif // WINDOW_H