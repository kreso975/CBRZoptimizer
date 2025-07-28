#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h> // For PathFindFileNameW if needed
#include <stdio.h>   // For swprintf, etc.
#include <stdlib.h>
#include <string.h>
#include "stb_image.h"

#include "functions.h"
#include "gui.h"
#include "webp_handle.h"
#include "debug.h"
#include <webp/encode.h>
#include <webp/decode.h>
#include <webp/types.h>  // Optional, if you use WebP-specific types

unsigned char* webp_decode_from_memory(const BYTE* buffer, DWORD size, int* width, int* height, int* channels)
{
    if (!buffer || size == 0 || !width || !height || !channels)
        return NULL;

    // Decode to RGB
    unsigned char* output = WebPDecodeRGB(buffer, size, width, height);
    if (!output)
        return NULL;

    *channels = 3; // RGB has 3 channels
    return output;
}

BOOL EncodeImageToWebPMemory(HWND hwnd, const uint8_t *data, int width, int height, BOOL useRGBA, uint8_t **out_data, size_t *out_size)
{
    WebPConfig config;
    if (!WebPConfigInit(&config))
    {
        DEBUG_PRINT(L"[WebP] ❌ Failed to init WebPConfig\n");
        return FALSE;
    }

    config.quality = (float)_wtoi(g_config.WebPQuality);
    config.method = _wtoi(g_config.WebPMethod);
    config.lossless = g_config.WebPLossless;
    config.thread_level = 1;
    config.exact = 1;

    WebPPicture pic;
    if (!WebPPictureInit(&pic))
    {
        DEBUG_PRINT(L"[WebP] ❌ Failed to init WebPPicture\n");
        return FALSE;
    }

    pic.width = width;
    pic.height = height;
    pic.use_argb = 1;

    BOOL imported = useRGBA ? WebPPictureImportRGBA(&pic, data, width * 4) : WebPPictureImportRGB(&pic, data, width * 3);

    if (!imported)
    {
        DEBUG_PRINT(L"[WebP] ❌ Failed to import image data\n");
        WebPPictureFree(&pic);
        return FALSE;
    }

    WebPMemoryWriter writer;
    WebPMemoryWriterInit(&writer);
    pic.writer = WebPMemoryWrite;
    pic.custom_ptr = &writer;

    if (!WebPEncode(&config, &pic))
    {
        wchar_t dbg[256];
        swprintf(dbg, _countof(dbg), L"[WebP] ❌ Encoding failed (error code: %d)\n", pic.error_code);
        DEBUG_PRINT(dbg);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, dbg, L"");
        WebPPictureFree(&pic);
        return FALSE;
    }

    *out_data = writer.mem;
    *out_size = writer.size;

    WebPPictureFree(&pic);
    return TRUE;
}

typedef struct {
    HWND hwnd;
    wchar_t image_path[MAX_PATH];
} WebPTask;

DWORD WINAPI ConvertToWebPThread(LPVOID lpParam)
{
    WebPTask *task = (WebPTask *)lpParam;
    wchar_t *full_path = task->image_path;
    HWND hwnd = task->hwnd;

    char utf8_path[MAX_PATH];
    if (WideCharToMultiByte(CP_UTF8, 0, full_path, -1, utf8_path, MAX_PATH, NULL, NULL) == 0)
    {
        free(task);
        return 1;
    }

    int width = 0, height = 0, channels = 0;
    unsigned char *img_data = stbi_load(utf8_path, &width, &height, &channels, 4);
    if (!img_data)
    {
        free(task);
        return 1;
    }

    DEBUG_PRINTF(L"[WebP] Encoding: %s | Quality=%d | Method=%d | Lossless=%d\n", 
                 full_path, _wtoi(g_config.WebPQuality), _wtoi(g_config.WebPMethod), g_config.WebPLossless);

    uint8_t *webp_data = NULL;
    size_t webp_size = 0;

    if (!EncodeImageToWebPMemory(hwnd, img_data, width, height, TRUE, &webp_data, &webp_size))
    {
        stbi_image_free(img_data);
        free(task);
        return 1;
    }

    stbi_image_free(img_data);

    if (webp_size == 0 || webp_data == NULL)
    {
        wchar_t dbg[512];
        swprintf(dbg, _countof(dbg), L"[WebP] ❌ Failed to encode: %s\n", full_path);
        DEBUG_PRINT(dbg);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, dbg, L"");
        free(task);
        return 1;
    }

    wchar_t output_path[MAX_PATH];
    wcscpy(output_path, full_path);
    wchar_t *dot = wcsrchr(output_path, L'.');
    if (dot)
        *dot = 0;
    wcscat(output_path, L".webp");

    FILE *fp = _wfopen(output_path, L"wb");
    if (fp)
    {
        fwrite(webp_data, 1, webp_size, fp);
        fclose(fp);
        DeleteFileW(full_path);

        wchar_t dbg[512];
        swprintf(dbg, _countof(dbg), L"[WebP] ✅ Converted: %s → .webp\n", full_path);
        DEBUG_PRINT(dbg);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, dbg, L"");
    }
    else
    {
        wchar_t dbg[512];
        swprintf(dbg, _countof(dbg), L"[WebP] ❌ Failed to write: %s\n", output_path);
        DEBUG_PRINT(dbg);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, dbg, L"");
    }

    WebPFree(webp_data);
    free(task);
    return 0;
}


static int webp_write_callback(const uint8_t* data, size_t data_size, const WebPPicture* const pic) {
    FILE* fp = (FILE*)pic->custom_ptr;
    return fwrite(data, 1, data_size, fp) == data_size;
}

BOOL webp_encode_and_write(HWND hwnd, const wchar_t *inputPath, const uint8_t *rgb_buf, int width, int height)
{
    wchar_t outputPath[MAX_PATH];
    wcsncpy_s(outputPath, MAX_PATH, inputPath, _TRUNCATE);

    const wchar_t *fileName = wcsrchr(inputPath, L'\\');
    fileName = fileName ? fileName + 1 : inputPath;

    wchar_t *ext = wcsrchr(outputPath, L'.');
    if (ext && _wcsicmp(ext, L".webp") != 0)
    {
        wcscpy_s(ext, MAX_PATH - (rsize_t)(ext - outputPath), L".webp");
        DEBUG_PRINTF(L"[WEBP] Replaced extension: %ls\n", outputPath);
    }
    else if (!ext)
    {
        size_t len = wcslen(outputPath);
        if (len + 5 < MAX_PATH)
        {
            wcscat_s(outputPath, MAX_PATH, L".webp");
            DEBUG_PRINTF(L"[WEBP] Appended extension: %ls\n", outputPath);
        }
        else
        {
            DEBUG_PRINT(L"[WEBP] File path too long to append extension.\n");
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", L"❌ Path too long.");
            return FALSE;
        }
    }

    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", fileName);

    uint8_t *webp_data = NULL;
    size_t webp_size = 0;

    if (!EncodeImageToWebPMemory(hwnd, rgb_buf, width, height, FALSE, &webp_data, &webp_size))
    {
        return FALSE;
    }

    FILE *outFile = _wfopen(outputPath, L"wb");
    if (!outFile)
    {
        DEBUG_PRINTF(L"[WEBP] Failed to open file for writing: %ls\n", outputPath);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", L"❌ Cannot open output file.");
        WebPFree(webp_data);
        return FALSE;
    }

    fwrite(webp_data, 1, webp_size, outFile);
    fclose(outFile);
    WebPFree(webp_data);

    DEBUG_PRINTF(L"[WEBP] Successfully wrote WebP file: %ls\n", outputPath);

    wchar_t status_msg[MAX_PATH + 64];
    swprintf(status_msg, _countof(status_msg), L"✅ Converted: %ls", fileName);
    SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", status_msg);

    return TRUE;
}

BOOL convert_images_to_webp(HWND hwnd, const wchar_t* folder_path)
{
    if (g_StopProcessing)
        return FALSE;

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    DWORD numCPU = sysinfo.dwNumberOfProcessors;
    DWORD max_threads = max(1, min(numCPU * 2, 64));

    HANDLE* threads = malloc(sizeof(HANDLE) * max_threads);
    int thread_count = 0;
    BOOL success = FALSE;

    WIN32_FIND_DATAW find_data;
    wchar_t search_path[MAX_PATH];
    swprintf(search_path, MAX_PATH, L"%s\\*.*", folder_path);

    HANDLE hFind = FindFirstFileW(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        free(threads);
        return FALSE;
    }

    do {
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        const wchar_t* ext = wcsrchr(find_data.cFileName, L'.');
        if (!ext) continue;

        if (_wcsicmp(ext, L".jpg") != 0 && _wcsicmp(ext, L".jpeg") != 0 && _wcsicmp(ext, L".png") != 0 && _wcsicmp(ext, L".bmp") != 0)
            continue;

        if (g_StopProcessing)
            break;

        WebPTask* task = (WebPTask*)malloc(sizeof(WebPTask));
        if (!task) continue;

        task->hwnd = hwnd;
        swprintf(task->image_path, MAX_PATH, L"%s\\%s", folder_path, find_data.cFileName);

        HANDLE hThread = CreateThread(NULL, 0, ConvertToWebPThread, task, 0, NULL);
        if (hThread) {
            threads[thread_count++] = hThread;
            success = TRUE;
        } else {
            free(task);
        }

        if ((DWORD)thread_count == max_threads) {
            WaitForMultipleObjects((DWORD)thread_count, threads, TRUE, INFINITE);
            for (int t = 0; t < thread_count; ++t)
                CloseHandle(threads[t]);
            thread_count = 0;
        }

    } while (FindNextFileW(hFind, &find_data));

    FindClose(hFind);

    if (thread_count > 0) {
        WaitForMultipleObjects((DWORD)thread_count, threads, TRUE, INFINITE);
        for (int t = 0; t < thread_count; ++t)
            CloseHandle(threads[t]);
    }

    free(threads);

    if (!g_StopProcessing)
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", L"✅ All images converted.");
    else
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", L"🛑 Conversion canceled.");

    return !g_StopProcessing && success;
}
