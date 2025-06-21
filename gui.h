#ifndef GUI_H
#define GUI_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define WM_UPDATE_TERMINAL_TEXT        (WM_USER + 1)
#define WM_UPDATE_PROCESSING_TEXT      (WM_USER + 2)

extern volatile BOOL g_StopProcessing;

extern HWND hTerminalProcessingLabel;
extern HWND hTerminalProcessingText;
extern HWND hTerminalText;

extern wchar_t TMP_FOLDER[MAX_PATH];
extern wchar_t OUTPUT_FOLDER[MAX_PATH];
extern wchar_t WINRAR_PATH[MAX_PATH];
extern wchar_t SEVEN_ZIP_PATH[MAX_PATH];
extern wchar_t IMAGEMAGICK_PATH[MAX_PATH];
extern wchar_t IMAGE_SIZE[];
extern wchar_t IMAGE_QUALITY[];
extern wchar_t IMAGE_DPI[];
extern BOOL g_RunImageOptimizer;
extern BOOL g_RunCompressor;
extern BOOL g_KeepExtracted;

#endif // GUI_H