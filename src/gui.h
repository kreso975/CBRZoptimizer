#ifndef GUI_H
#define GUI_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ─────────────── Custom Messages ───────────────
#define WM_UPDATE_TERMINAL_TEXT   (WM_USER + 1)
#define WM_UPDATE_PROCESSING_TEXT (WM_USER + 2)
#define WM_PROCESSING_DONE        (WM_USER + 3)

extern volatile BOOL g_StopProcessing;
extern HBITMAP hButtonPlus, hButtonMinus, hButtonBrowse, hButtonStart, hButtonStop, hButtonAddFolder, hButtonOpenInFolder;
extern HWND hStartButton, hStopButton, hRemoveButton, hAddButton, hAddFolderButton, hOpenInTmpFolderButton, hOpenInOutputFolderButton;
extern HWND hTmpFolder, hOutputFolder, hWinrarPath, hSevenZipPath, hImageMagickPath, hMuToolPath, hImageResize;
extern HWND hTmpBrowse, hOutputBrowse, hWinrarBrowse, hSevenZipBrowse, hImageMagickBrowse, hMuToolBrowse;
extern HWND hTerminalProcessingLabel, hTerminalProcessingText, hTerminalText;
extern HWND hTmpFolderLabel, hOutputFolderLabel, hWinrarLabel, hSevenZipLabel, hMuToolLabel;
extern HWND hImageTypeLabel, hImageAllowUpscalingLabel, hImageResizeToLabel, hImageMagickLabel;
extern HWND hImageQualityLabel, hImageQualityValue, hImageSizeWidthLabel, hImageSizeHeightLabel, hImageKeepAspectRatioLabel;

extern HWND hImageType, hImageAllowUpscaling, hImageResizeTo, hImageQualitySlider, hImageSizeWidth, hImageSizeHeight, hImageKeepAspectRatio;
extern HWND hImageSizeWidth, hImageSizeHeight, hImageKeepAspectRatio;
// Output options
extern HWND hOutputKeepExtractedLabel, hOutputKeepExtracted, hOutputKeepExtractedLabel, hOutputExtractCover, hOutputExtractCoverLabel, hOutputRunExtractLabel, hOutputRunExtract;
extern HWND hOutputType, hOutputTypeLabel, hOutputRunImageOptimizer, hOutputRunCompressor, hOutputRunImageOptimizerLabel, hOutputRunCompressorLabel;

// ⚙️ ✨ ℹ️✔️ ❗ ❓✏️☕ ☀️ ☁️ ☂️ ☃️ ☔️ ⚡️ ❄️ ☑️ ☢️ ☣️ ☠️ ☤ ☮️ ☯️ ☸️ ☹️ ☺️ ⚓ ⚡⚪
// ⚫ ⚽ ⚾ ⛄ ⛅ ⛔ ⛪ ⛲ ⛳ ⛵ ⛹ ⛺ ⛽ ✅ ✉ ✊ ✋ ✌ ✍ ✨ ✪
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
    wchar_t MUTOOL_PATH[MAX_PATH];

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
    BOOL extractCover;
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
    BOOL triggersGroupLogic; // If TRUE, toggling this checkbox will trigger group logic listen for clicks on label and checkbox
} LabelCheckboxPair;

extern LabelCheckboxPair controls[];
extern const size_t controlCount;

typedef struct {
    const wchar_t *labelText;
    const wchar_t *configKey;
    const wchar_t *configSection;
    wchar_t *defaultText;
    int y;
    int labelWidth;
    int editWidth;
    HWND *hLabel;
    HWND *hEdit;
    const wchar_t *tooltipText;
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

// ─────────────── Image Field Binding Struct ───────────────
// Represents a binding between an INI key and its associated UI control and config field.
typedef struct {
    const wchar_t *key;        // INI key name
    wchar_t *target;           // Pointer to config field
    DWORD size;                // Max buffer size
    HWND *hwnd;                // Associated UI control
    BOOL isSlider;             // TRUE if value should update a slider
    BOOL isDropdown;           // TRUE if value should update a dropdown
} ImageFieldBinding;

// External declaration of image field bindings
extern ImageFieldBinding imageFields[];
extern const size_t imageFieldsCount;

// ─────────────── Button Image Struct ───────────────
typedef struct
{
    HWND *handle;
    int x, y;
    UINT id;
    int bmpId;
    const wchar_t *tooltip;
    HBITMAP *bmpHandle;
    int imageW, imageH;
    BOOL isHover; // Hover state stored directly
    BOOL visible; // field to control initial visibility
} ButtonSpec;

// External declaration of the array and its count
extern ButtonSpec buttons[];
extern const size_t buttonsCount;

// ─────────────── Utility Prototypes ───────────────
void SetControlsEnabled(BOOL enable, int count, ...);
void EnableResizeGroupWithLogic(LPCWSTR groupName, BOOL enable);
int  MessageBoxCentered(HWND hwnd, LPCWSTR text, LPCWSTR caption, UINT type);
void AdjustLayout(HWND hwnd);
void ValidateAndSaveInput(HWND hwnd, HWND changedControl, const wchar_t *iniPath);
void SendStatus(HWND hwnd, UINT messageId, const wchar_t *prefix, const wchar_t *info);
void AddUniqueToListBox(HWND hwndOwner, HWND hListBox, LPCWSTR itemText);
void ProcessDroppedFiles(HWND hwnd, HWND hListBox, HDROP hDrop);
void RemoveSelectedItems(HWND hListBox);
void update_output_type_dropdown();
void load_config_values(void);

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);

#endif // GUI_H