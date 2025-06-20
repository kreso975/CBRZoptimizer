#include <windows.h>
#include <wchar.h>

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow)
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeW(path, &dummy);
    if (size == 0) {
        MessageBoxW(NULL, L"VERSIONINFO missing from EXE", L"Version Check", MB_OK);
        return 0;
    }

    MessageBoxW(NULL, L"VERSIONINFO found!", L"Version Check", MB_OK);
    return 0;
}
