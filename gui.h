#ifndef GUI_H
#define GUI_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define WM_UPDATE_TERMINAL_TEXT (WM_USER + 1)
#define WM_UPDATE_PROCESSING_TEXT (WM_USER + 2)

extern volatile BOOL g_StopProcessing;

extern HWND hTerminalProcessingLabel;
extern HWND hTerminalProcessingText;
extern HWND hTerminalText;

extern wchar_t IMAGE_SIZE_WIDTH[];
extern wchar_t IMAGE_SIZE_HEIGHT[];
extern wchar_t IMAGE_QUALITY[];

typedef struct {
    const wchar_t  *labelText;
    const wchar_t  *configKey;
    int y;
    HWND *hCheckbox;
    HWND *hLabel;
} LabelCheckboxPair;

// ────────────────────────────────────────────────────────────────────────────
// bundle every HWND you care about under one roof
// ────────────────────────────────────────────────────────────────────────────
#define IMAGE_TYPE_LEN 10
#define IMG_DIM_LEN 16
#define QUALITY_LEN 8

typedef struct
{
   // Paths
   wchar_t TMP_FOLDER[MAX_PATH];
   wchar_t OUTPUT_FOLDER[MAX_PATH];
   wchar_t WINRAR_PATH[MAX_PATH];
   wchar_t IMAGEMAGICK_PATH[MAX_PATH];
   wchar_t SEVEN_ZIP_PATH[MAX_PATH];

   // Image settings
   wchar_t IMAGE_TYPE[IMAGE_TYPE_LEN];
   BOOL resizeTo;        // was g_ResizeTo
   BOOL keepAspectRatio; // was g_KeepAspectRatio
   wchar_t IMAGE_SIZE_WIDTH[IMG_DIM_LEN];
   wchar_t IMAGE_SIZE_HEIGHT[IMG_DIM_LEN];
   wchar_t IMAGE_QUALITY[QUALITY_LEN];

   // Output flags
   BOOL runImageOptimizer; // was g_RunImageOptimizer
   BOOL runCompressor;     // was g_RunCompressor
   BOOL keepExtracted;     // was g_KeepExtracted
} AppConfig;

// single global instance:
extern AppConfig g_config;

#endif // GUI_H