#ifndef GUI_H
#define GUI_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ─────────────── Custom Messages ───────────────
#define WM_UPDATE_TERMINAL_TEXT   (WM_USER + 1)
#define WM_UPDATE_PROCESSING_TEXT (WM_USER + 2)
#define WM_PROCESSING_DONE        (WM_USER + 3)

extern volatile BOOL g_StopProcessing;
extern HBITMAP hButtonPlus, hButtonMinus, hButtonBrowse, hButtonStart, hButtonStop;
extern HWND hTmpFolder, hOutputFolder, hWinrarPath, hSevenZipPath, hImageMagickPath, hImageResize;
extern HWND hTmpBrowse, hOutputBrowse, hWinrarBrowse, hSevenZipBrowse, hImageMagickBrowse;
extern HWND hTerminalProcessingLabel, hTerminalProcessingText, hTerminalText;
extern HWND hTmpFolderLabel, hOutputFolderLabel, hWinrarLabel, hSevenZipLabel;
extern HWND hImageTypeLabel, hImageAllowUpscalingLabel, hImageResizeToLabel, hImageMagickLabel;
extern HWND hImageQualityLabel, hImageQualityValue, hImageSizeWidthLabel, hImageSizeHeightLabel, hImageKeepAspectRatioLabel;


// ─────────────── Config Structure ───────────────
#define IMAGE_TYPE_LEN 10
#define IMG_DIM_LEN    16
#define QUALITY_LEN    8

typedef struct {
    // Paths
    wchar_t TMP_FOLDER[MAX_PATH];
    wchar_t OUTPUT_FOLDER[MAX_PATH];
    wchar_t WINRAR_PATH[MAX_PATH];
    wchar_t IMAGEMAGICK_PATH[MAX_PATH];
    wchar_t SEVEN_ZIP_PATH[MAX_PATH];

    // Image settings
    wchar_t IMAGE_TYPE[IMAGE_TYPE_LEN];
    BOOL resizeTo;
    BOOL keepAspectRatio;
    BOOL allowUpscaling;
    wchar_t IMAGE_SIZE_WIDTH[IMG_DIM_LEN];
    wchar_t IMAGE_SIZE_HEIGHT[IMG_DIM_LEN];
    wchar_t IMAGE_QUALITY[QUALITY_LEN];

    // Output flags
    BOOL runImageOptimizer;
    BOOL runCompressor;
    BOOL keepExtracted;
} AppConfig;

extern AppConfig g_config;

// ─────────────── Checkbox Binding Struct ───────────────
typedef struct {
    const wchar_t *labelText;
    const wchar_t *configKey;
    const wchar_t *configSegment;
    int y;
    HWND *hCheckbox;
    HWND *hLabel;
    BOOL *configField;
} LabelCheckboxPair;

// ─────────────── Browse Field Struct ───────────────
typedef struct {
    const wchar_t *labelText;      // Label displayed next to the field
    const wchar_t *configKey;      // INI key name
    const wchar_t *configSection;  // INI section name (e.g. "Paths")
    wchar_t *defaultText;          // Pointer to current config value (e.g. g_config.FIELD)
    int y;
    int labelWidth;
    int editWidth;
    int browseId;
    HWND *hLabel;
    HWND *hEdit;
    HWND *hBrowse;
    HBITMAP *hBitmap;
    const wchar_t *bmpPath;
} EditBrowseControl;

extern EditBrowseControl inputs[];
extern const size_t inputsCount;

// ─────────────── Grouped UI Handles ───────────────
typedef struct {
    LPCWSTR name;
    LPCWSTR group;
    HWND *hwndPtr;
} GUIHandleEntry;

extern GUIHandleEntry groupElements[];
extern int groupElementsCount;

// ─────────────── Utility Prototypes ───────────────
void EnableGroupElements(LPCWSTR groupName, BOOL enable);
int  MessageBoxCentered(HWND hwnd, LPCWSTR text, LPCWSTR caption, UINT type);
void AdjustLayout(HWND hwnd);
void ToggleResizeImageCheckbox(void);
void ValidateAndSaveInput(HWND changedControl, const wchar_t *iniPath);

#endif // GUI_H