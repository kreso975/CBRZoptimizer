#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include "gui.h"
#include "resource.h"
#include "functions.h"
#include "window.h"


EditBrowseControl inputs[] = {
    { L"Temp Folder:",     L"TMP_FOLDER",       L"Paths", g_config.TMP_FOLDER,     30, 100, 200, ID_TMP_FOLDER_BROWSE,     &hTmpFolderLabel,     &hTmpFolder,     &hTmpBrowse,      &hButtonBrowse,     NULL },
    { L"Output Folder:",   L"OUTPUT_FOLDER",    L"Paths", g_config.OUTPUT_FOLDER,  60, 100, 200, ID_OUTPUT_FOLDER_BROWSE,  &hOutputFolderLabel,  &hOutputFolder,  &hOutputBrowse,   &hButtonBrowse,     NULL },
    { L"WinRAR Path:",     L"WINRAR_PATH",      L"Paths", g_config.WINRAR_PATH,    90, 100, 200, ID_WINRAR_PATH_BROWSE,    &hWinrarLabel,        &hWinrarPath,    &hWinrarBrowse,   &hButtonBrowse,     NULL },
    { L"7-Zip Path:",      L"SEVEN_ZIP_PATH",   L"Paths", g_config.SEVEN_ZIP_PATH, 120, 100, 200, ID_SEVEN_ZIP_PATH_BROWSE, &hSevenZipLabel,      &hSevenZipPath,  &hSevenZipBrowse, &hButtonBrowse,     NULL },
    { L"ImageMagick:",     L"IMAGEMAGICK_PATH", L"Paths", g_config.IMAGEMAGICK_PATH,150, 100, 200, ID_IMAGEMAGICK_PATH_BROWSE,&hImageMagickLabel,  &hImageMagickPath,&hImageMagickBrowse,&hButtonBrowse,    NULL }
};
const size_t inputsCount = sizeof(inputs) / sizeof(inputs[0]);

