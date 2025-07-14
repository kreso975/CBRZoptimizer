#include "folder_handle.h"
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <wchar.h>



BOOL copy_folder_to_tmp(const wchar_t *srcFolder, const wchar_t *dstFolder)
{
    WIN32_FIND_DATAW findData;
    wchar_t searchPath[MAX_PATH];
    _snwprintf(searchPath, MAX_PATH, L"%s\\*", srcFolder);

    HANDLE hFind = FindFirstFileW(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    CreateDirectoryW(dstFolder, NULL);

    do
    {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;

        wchar_t srcPath[MAX_PATH], dstPath[MAX_PATH];
        swprintf(srcPath, MAX_PATH, L"%s\\%s", srcFolder, findData.cFileName);
        swprintf(dstPath, MAX_PATH, L"%s\\%s", dstFolder, findData.cFileName);

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Recursively copy subfolder
            if (!copy_folder_to_tmp(srcPath, dstPath))
            {
                FindClose(hFind);
                return FALSE;
            }
        }
        else
        {
            // Copy file
            if (!CopyFileW(srcPath, dstPath, FALSE))
            {
                FindClose(hFind);
                return FALSE;
            }
        }

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return TRUE;
}