#ifndef GUI_H
#define GUI_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define WM_UPDATE_TERMINAL_TEXT (WM_USER + 1)
#define WM_UPDATE_PROCESSING_TEXT (WM_USER + 2)
#define WM_PROCESSING_DONE (WM_USER + 3)

extern volatile BOOL g_StopProcessing;

typedef struct
{
    const wchar_t *labelText;
    const wchar_t *configKey;
    const wchar_t *configSegment;
    int y;
    HWND *hCheckbox;
    HWND *hLabel;
    BOOL *configField;
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
    BOOL allowUpscaling;
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

typedef struct
{
    LPCWSTR name;
    LPCWSTR group;
    HWND *hwndPtr;
} GUIHandleEntry;

extern GUIHandleEntry groupElements[];
extern int groupElementsCount;

void EnableGroupElements(LPCWSTR groupName, BOOL enable);
int MessageBoxCentered(HWND hwnd, LPCWSTR text, LPCWSTR caption, UINT type);

void AdjustLayout(HWND hwnd);
void ToggleResizeImageCheckbox(void);

#endif // GUI_H