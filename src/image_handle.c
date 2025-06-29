#include <windows.h>
#include <shlobj.h>  // For SHFileOperation
#include <shlwapi.h> // If not already present
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>
#include <wchar.h>

#include "image_handle.h"
#include "gui.h"
#include "resource.h"
#include "functions.h" // For SendStatus, IMAGE_QUALITY, etc.
#include "debug.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wshadow"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"

#pragma GCC diagnostic pop

// STB safe write callback
void stb_write_func(void *context, void *data, int size)
{
    FILE *fp = (FILE *)context;
    fwrite(data, 1, (size_t)size, fp);
}

DWORD WINAPI OptimizeImageThread(LPVOID lpParam)
{
    ImageTask *task = (ImageTask *)lpParam;
    wchar_t *pathW = task->image_path;
    char utf8_path[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, pathW, -1, utf8_path, MAX_PATH, NULL, NULL);

    // 1) Load file into memory
    HANDLE hFile = CreateFileW(pathW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"❌ Unable to open file.");
        free(task);
        return 1;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    BYTE *fileBuf = (BYTE *)malloc(fileSize);
    if (!fileBuf)
    {
        CloseHandle(hFile);
        SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"❌ Memory allocation failed.");
        free(task);
        return 1;
    }

    DWORD bytesRead;
    ReadFile(hFile, fileBuf, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    // 2) Decode with STB
    int w, h, c;
    unsigned char *input = stbi_load_from_memory(fileBuf, (int)fileSize, &w, &h, &c, 3);
    free(fileBuf);

    if (!input)
    {
        SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"❌ Failed to decode image.");
        free(task);
        return 1;
    }

    // 3) Build output path (.bmp → .jpg)
    wchar_t outPath[MAX_PATH];
    wcscpy_s(outPath, MAX_PATH, pathW);

    const wchar_t *ext = PathFindExtensionW(pathW);
    BOOL isBMP = (ext && _wcsnicmp(ext, L".bmp", 4) == 0);

    if (isBMP)
    {
        wchar_t *dot = PathFindExtensionW(outPath);
        if (dot && *dot)
        {
            wcscpy_s(dot, (rsize_t)(MAX_PATH - (dot - outPath)), L".jpg");
            DEBUG_PRINTF(L"[STB] 🖼 BMP detected — remapped to: %ls\n", outPath);
        }
    }

    const wchar_t *outExt = PathFindExtensionW(outPath);

    // 4) Resize decision
    BOOL should_resize = TRUE;
    int newW = w, newH = h;

    if (!task->allow_upscale)
    {
        if (task->keep_aspect)
        {
            if ((wcscmp(g_config.IMAGE_TYPE, L"Portrait") == 0 && h <= task->target_height) ||
                (wcscmp(g_config.IMAGE_TYPE, L"Landscape") == 0 && w <= task->target_width))
                should_resize = FALSE;
        }
        else
        {
            if (w <= task->target_width && h <= task->target_height)
                should_resize = FALSE;
        }
    }

    if (!task->keep_aspect && w == task->target_width && h == task->target_height)
        should_resize = FALSE;

    unsigned char *final_buf = input;
    unsigned char *resized = NULL;

    // 5) Resize if needed
    if (should_resize)
    {
        DEBUG_PRINTF(L"[STB] 🔄 Resizing %dx%d → target %dx%d\n", w, h, task->target_width, task->target_height);

        if (task->keep_aspect)
        {
            float aspect = (float)w / (float)h;

            if (wcscmp(g_config.IMAGE_TYPE, L"Portrait") == 0 && task->target_height > 0)
            {
                newH = task->target_height;
                newW = max(1, (int)((float)newH * aspect));
            }
            else if (wcscmp(g_config.IMAGE_TYPE, L"Landscape") == 0 && task->target_width > 0)
            {
                newW = task->target_width;
                newH = max(1, (int)((float)newW / aspect));
            }
            else
            {
                float scaleW = (float)task->target_width / (float)w;
                float scaleH = (float)task->target_height / (float)h;
                float scale = (scaleW < scaleH) ? scaleW : scaleH;
                newW = max(1, (int)((float)w * scale));
                newH = max(1, (int)((float)h * scale));
            }
        }
        else
        {
            newW = task->target_width;
            newH = task->target_height;
        }

        size_t bufSize = (size_t)newW * (size_t)newH * 3;
        resized = (unsigned char *)malloc(bufSize);
        if (!resized)
        {
            stbi_image_free(input);
            SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"❌ Memory allocation failed.");
            free(task);
            return 1;
        }

        stbir_resize_uint8_linear(input, w, h, 0, resized, newW, newH, 0, STBIR_RGB);
        stbi_image_free(input);
        final_buf = resized;
    }
    else
    {
        DEBUG_PRINTF(L"[STB] ⏭ Skipping resize: keeping %dx%d\n", w, h);
    }

    // 6) Write output
    int result = 0;
    FILE *fp = _wfopen(outPath, L"wb");
    if (fp)
    {
        DEBUG_PRINTF(L"[STB] 💾 Saving to %ls\n", outPath);
        if (outExt && _wcsicmp(outExt, L".jpg") == 0)
            result = stbi_write_jpg_to_func(stb_write_func, fp, newW, newH, 3, final_buf, _wtoi(g_config.IMAGE_QUALITY));
        else if (outExt && _wcsicmp(outExt, L".png") == 0)
            result = stbi_write_png_to_func(stb_write_func, fp, newW, newH, 3, final_buf, newW * 3);
        fclose(fp);
    }

    // 7) Delete .bmp if converted
    if (result && isBMP)
    {
        if (DeleteFileW(pathW))
            DEBUG_PRINTF(L"[STB] 🧹 Deleted original BMP: %ls\n", pathW);
        else
            DEBUG_PRINTF(L"[STB] ⚠ Failed to delete BMP: %ls (Error: %lu)\n", pathW, GetLastError());
    }

    // 8) Cleanup
    if (resized)
        free(resized);
    else
        stbi_image_free(input);

    SendStatus(task->hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", result ? L"✔ Image optimized and saved." : L"⚠ Failed to write image.");

    free(task);
    return 0;
}

// Fallback Image Optimization using STB
BOOL fallback_optimize_images(HWND hwnd, const wchar_t *folder)
{
    if (g_StopProcessing)
        return FALSE;

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    DWORD numCPU = sysinfo.dwNumberOfProcessors;
    DWORD max_threads = max(1, min(numCPU * 2, 64));

    const wchar_t *exts[] = {L"jpg", L"png", L"bmp"};
    HANDLE *threads = malloc(sizeof(HANDLE) * max_threads);
    int thread_count = 0;

    wchar_t search_path[MAX_PATH], image_path[MAX_PATH];

    int target_width = _wtoi(g_config.IMAGE_SIZE_WIDTH);
    int target_height = _wtoi(g_config.IMAGE_SIZE_HEIGHT);
    BOOL keep_aspect = g_config.keepAspectRatio;
    BOOL allow_upscale = g_config.allowUpscaling;

    // Debug: dump global config into Output
    wchar_t dbg[256];
    swprintf(dbg, _countof(dbg), L"[STB] ResizeTo: %d | Width: %d | Height: %d | Aspect: %d | Upscale: %d\n",
             g_config.resizeTo, target_width, target_height, keep_aspect, allow_upscale);
    DEBUG_PRINT(dbg);

    for (size_t i = 0; i < ARRAYSIZE(exts); i++)
    {
        swprintf(search_path, MAX_PATH, L"%s\\*.%s", folder, exts[i]);

        WIN32_FIND_DATAW ffd;
        HANDLE hFind = FindFirstFileW(search_path, &ffd);
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            if (g_StopProcessing)
                break;

            swprintf(image_path, MAX_PATH, L"%s\\%s", folder, ffd.cFileName);

            ImageTask *task = malloc(sizeof(ImageTask));
            if (!task)
                continue;

            wcscpy(task->image_path, image_path);
            task->hwnd = hwnd;
            task->target_width = target_width;
            task->target_height = target_height;
            task->keep_aspect = keep_aspect;
            task->allow_upscale = allow_upscale;

            threads[thread_count++] = CreateThread(NULL, 0, OptimizeImageThread, task, 0, NULL);

            if ((DWORD)thread_count == max_threads)
            {
                WaitForMultipleObjects((DWORD)thread_count, threads, TRUE, INFINITE);
                for (int t = 0; t < thread_count; t++)
                    CloseHandle(threads[t]);
                thread_count = 0;
            }

        } while (FindNextFileW(hFind, &ffd));

        FindClose(hFind);
        if (g_StopProcessing)
            break;
    }

    if (thread_count > 0)
    {
        WaitForMultipleObjects((DWORD)thread_count, threads, TRUE, INFINITE);
        for (int t = 0; t < thread_count; t++)
            CloseHandle(threads[t]);
    }

    free(threads);

    if (!g_StopProcessing)
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"✅ All formats processed.");
    else
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"STB fallback: ", L"🛑 Processing canceled.");

    return !g_StopProcessing;
}

// Optimize Images
BOOL optimize_images(HWND hwnd, const wchar_t *image_folder)
{
    wchar_t command[MAX_PATH * 2], buffer[4096];

    DWORD bytesRead;
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    STARTUPINFOW si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;

    TrimTrailingWhitespace(g_config.IMAGEMAGICK_PATH);

    // Fallback early if IMAGEMAGICK_PATH is missing or invalid
    if (wcslen(g_config.IMAGEMAGICK_PATH) == 0 || GetFileAttributesW(g_config.IMAGEMAGICK_PATH) == INVALID_FILE_ATTRIBUTES ||
        (GetFileAttributesW(g_config.IMAGEMAGICK_PATH) & FILE_ATTRIBUTE_DIRECTORY))
    {
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"Image optimization: ", L"Falling back to STB optimizer...");
        DEBUG_PRINT(g_config.runImageOptimizer ? L"ImageOptimizer1: ON\n" : L"ImageOptimizer1: OFF\n");
        return fallback_optimize_images(hwnd, image_folder);
    }

    wchar_t resizeArg[64] = L"";
    BOOL hasWidth = wcslen(g_config.IMAGE_SIZE_WIDTH) > 0;
    BOOL hasHeight = wcslen(g_config.IMAGE_SIZE_HEIGHT) > 0;

    if (g_config.resizeTo)
    {
        const wchar_t *resizeSuffix = g_config.allowUpscaling ? L"" : L">";

        if (g_config.keepAspectRatio)
        {
            if (wcscmp(g_config.IMAGE_TYPE, L"Portrait") == 0 && hasHeight)
                swprintf(resizeArg, _countof(resizeArg), L"-resize x%s%s", g_config.IMAGE_SIZE_HEIGHT, resizeSuffix);
            else if (wcscmp(g_config.IMAGE_TYPE, L"Landscape") == 0 && hasWidth)
                swprintf(resizeArg, _countof(resizeArg), L"-resize %s%s", g_config.IMAGE_SIZE_WIDTH, resizeSuffix);
        }
        else if (hasWidth && hasHeight)
        {
            // For forced resize, don't apply conditional suffix
            swprintf(resizeArg, _countof(resizeArg), L"-resize %sx%s!", g_config.IMAGE_SIZE_WIDTH, g_config.IMAGE_SIZE_HEIGHT);
        }
    }

    const wchar_t *exts[] = {L"jpg", L"png"};
    //DEBUG_PRINT(g_config.runImageOptimizer ? L"ImageOptimizer: ON\n" : L"ImageOptimizer: OFF\n");
    for (int i = 0; i < 2; i++)
    {
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
        {
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", L"Pipe creation failed. Using STB fallback.");
            return fallback_optimize_images(hwnd, image_folder);
        }

        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;
        si.wShowWindow = SW_HIDE;
        si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

        // swprintf(command, MAX_PATH, L"\"%s\" mogrify -resize %s -quality %s \"%s\\*.%s\"",
        //          g_config.IMAGEMAGICK_PATH, g_config.IMAGE_SIZE_HEIGHT, g_config.IMAGE_QUALITY, image_folder, exts[i]);

        swprintf(command, MAX_PATH * 2, L"\"%s\" mogrify %s -quality %s \"%s\\*.%s\"",
                 g_config.IMAGEMAGICK_PATH,
                 resizeArg[0] != L'\0' ? resizeArg : L"",
                 g_config.IMAGE_QUALITY,
                 image_folder,
                 exts[i]);
        DEBUG_PRINT(command);

        if (!CreateProcessW(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        {
            CloseHandle(hWritePipe);
            CloseHandle(hReadPipe);
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", L"Failed to execute. Falling back to STB.");
            return fallback_optimize_images(hwnd, image_folder);
        }

        CloseHandle(hWritePipe);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", command);

        if (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", buffer);
        }
        else
        {
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", L"No output captured.");
        }

        CloseHandle(hReadPipe);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"ImageMagick: ", L"✔ All formats optimized.");
    return TRUE;
}