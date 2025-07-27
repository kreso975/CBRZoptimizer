#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h> // For PathFindFileNameW if needed
#include <stdio.h>   // For swprintf, etc.
#include <stdlib.h>
#include <string.h>

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

BOOL webp_encode_and_write(const wchar_t *inputPath, const uint8_t *rgb_buf, int width, int height)
{
    WebPPicture pic;
    WebPConfig config;

    // Create output path by replacing or appending .webp
    wchar_t outputPath[MAX_PATH];
    wcsncpy_s(outputPath, MAX_PATH, inputPath, _TRUNCATE);

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
            return FALSE;
        }
    }

    FILE *outFile = _wfopen(outputPath, L"wb");
    if (!outFile)
    {
        DEBUG_PRINTF(L"[WEBP] Failed to open file for writing: %ls\n", outputPath);
        return FALSE;
    }

    if (!WebPPictureInit(&pic) || !WebPConfigInit(&config))
    {
        DEBUG_PRINT(L"[WEBP] Failed to initialize WebP structures.\n");
        fclose(outFile);
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
    }
    else
    {
        DEBUG_PRINTF(L"[WEBP] Encoding failed with error code: %d\n", pic.error_code);
    }

    return success;
}