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


static int webp_write_callback(const uint8_t* data, size_t data_size, const WebPPicture* const pic) {
    FILE* fp = (FILE*)pic->custom_ptr;
    return fwrite(data, 1, data_size, fp) == data_size;
}

uint8_t* webp_encode_image(const uint8_t* rgba, int width, int height, int stride, int quality, size_t* output_size) {
    if (!rgba || width <= 0 || height <= 0 || !output_size) return NULL;

    uint8_t* output_data = NULL;
    float quality_factor = (float)quality;

    size_t size = WebPEncodeRGBA(rgba, width, height, stride, quality_factor, &output_data);
    if (size == 0 || output_data == NULL) return NULL;

    *output_size = size;
    return output_data;
}


uint8_t* webp_decode_image(const uint8_t* webp_data, size_t webp_size, int* width, int* height) {
    if (!webp_data || webp_size == 0 || !width || !height) {
        return NULL;
    }

    return WebPDecodeRGBA(webp_data, webp_size, width, height);
}

BOOL webp_encode_and_write(HWND hwnd, const wchar_t *inputPath, const uint8_t *rgb_buf, int width, int height)
{
    WebPPicture pic;
    WebPConfig config;

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

    FILE *outFile = _wfopen(outputPath, L"wb");
    if (!outFile)
    {
        DEBUG_PRINTF(L"[WEBP] Failed to open file for writing: %ls\n", outputPath);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", L"❌ Cannot open output file.");
        return FALSE;
    }

    if (!WebPPictureInit(&pic) || !WebPConfigInit(&config))
    {
        DEBUG_PRINT(L"[WEBP] Failed to initialize WebP structures.\n");
        fclose(outFile);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", L"❌ Init failed.");
        return FALSE;
    }

    config.quality = (float)_wtoi(g_config.WebPQuality);
    config.method = _wtoi(g_config.WebPMethod);
    config.lossless = g_config.WebPLossless;

    DEBUG_PRINTF(L"[WEBP] Config - Quality: %.2f | Method: %d | Lossless: %d\n",
                 (double)config.quality, config.method, config.lossless);

    pic.width = width;
    pic.height = height;
    pic.use_argb = 1;

    DEBUG_PRINTF(L"[WEBP] Picture - Width: %d | Height: %d\n", width, height);

    if (!WebPPictureImportRGB(&pic, rgb_buf, width * 3))
    {
        DEBUG_PRINT(L"[WEBP] Failed to import RGB buffer.\n");
        WebPPictureFree(&pic);
        fclose(outFile);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", L"❌ Import failed.");
        return FALSE;
    }

    pic.writer = webp_write_callback;
    pic.custom_ptr = outFile;

    BOOL success = WebPEncode(&config, &pic);
    WebPPictureFree(&pic);
    fclose(outFile);

    if (success)
    {
        DEBUG_PRINTF(L"[WEBP] Successfully wrote WebP file: %ls\n", outputPath);

        wchar_t status_msg[MAX_PATH + 64];
        swprintf(status_msg, _countof(status_msg), L"✅ Converted: %ls", fileName);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", status_msg);
    }
    else
    {
        DEBUG_PRINTF(L"[WEBP] Encoding failed with error code: %d\n", pic.error_code);

        wchar_t status_msg[MAX_PATH + 64];
        swprintf(status_msg, _countof(status_msg), L"❌ Failed: %ls", fileName);
        SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, L"WebP: ", status_msg);
    }

    return success;
}

BOOL convert_images_to_webp(HWND hwnd, const wchar_t* folder_path)
{
    WIN32_FIND_DATAW find_data;
    wchar_t search_path[MAX_PATH];
    swprintf(search_path, MAX_PATH, L"%s\\*.*", folder_path);

    HANDLE hFind = FindFirstFileW(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    BOOL success = FALSE;

    do {
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        const wchar_t* ext = wcsrchr(find_data.cFileName, L'.');
        if (!ext) continue;

        if (_wcsicmp(ext, L".jpg") != 0 && _wcsicmp(ext, L".jpeg") != 0 &&
            _wcsicmp(ext, L".png") != 0 && _wcsicmp(ext, L".bmp") != 0)
            continue;

        wchar_t full_path[MAX_PATH];
        swprintf(full_path, MAX_PATH, L"%s\\%s", folder_path, find_data.cFileName);

        // Convert wchar_t path to UTF-8 for STB
        char utf8_path[MAX_PATH];
        int len = WideCharToMultiByte(CP_UTF8, 0, full_path, -1, utf8_path, MAX_PATH, NULL, NULL);
        if (len == 0)
            continue;

        int width = 0, height = 0, channels = 0;
        unsigned char* img_data = stbi_load(utf8_path, &width, &height, &channels, 4); // force RGBA
        if (!img_data)
            continue;

        // 🧠 Debug info
        wchar_t dbg[512];
        swprintf(dbg, _countof(dbg), L"[WebP] File: %s | Size: %dx%d | Channels: %d\n", find_data.cFileName, width, height, channels);
        DEBUG_PRINT(dbg);

        uint8_t* webp_data = NULL;
        size_t webp_size = WebPEncodeRGBA(img_data, width, height, width * 4, 75, &webp_data); // quality = 75

        stbi_image_free(img_data);

        if (webp_size == 0 || webp_data == NULL)
        {
            swprintf(dbg, _countof(dbg), L"[WebP] ❌ Failed to encode: %s\n", find_data.cFileName);
            DEBUG_PRINT(dbg);
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, dbg, L"");
            continue;
        }

        wchar_t output_path[MAX_PATH];
        wcscpy(output_path, full_path);
        wchar_t* dot = wcsrchr(output_path, L'.');
        if (dot) *dot = 0;
        wcscat(output_path, L".webp");

        FILE* fp = _wfopen(output_path, L"wb");
        if (fp) {
            fwrite(webp_data, 1, webp_size, fp);
            fclose(fp);

            DeleteFileW(full_path);
            success = TRUE;

            swprintf(dbg, _countof(dbg), L"[WebP] ✅ Converted: %s → .webp\n", find_data.cFileName);
            DEBUG_PRINT(dbg);
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, dbg, L"");
        } else {
            swprintf(dbg, _countof(dbg), L"[WebP] ❌ Failed to write: %s\n", output_path);
            DEBUG_PRINT(dbg);
            SendStatus(hwnd, WM_UPDATE_TERMINAL_TEXT, dbg, L"");
        }

        WebPFree(webp_data);

    } while (FindNextFileW(hFind, &find_data));

    FindClose(hFind);
    return success;
}

