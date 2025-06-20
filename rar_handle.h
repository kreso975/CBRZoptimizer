#pragma once

#include <windows.h>

typedef struct {
    const char *ArcName;
    const wchar_t *ArcNameW;
    unsigned int OpenMode;
    unsigned int OpenResult;
    const char *CmtBuf;
    const wchar_t *CmtBufW;
    unsigned int CmtBufSize;
    unsigned int CmtSize;
    unsigned int CmtState;
    unsigned int Flags;
    unsigned int Reserved[32];
    void *Callback;
    LPARAM UserData;
    const wchar_t *MarkOfTheWeb;
} RAROpenArchiveDataEx;

typedef struct {
    char ArcName[1024];
    wchar_t ArcNameW[1024];
    char FileName[1024];
    wchar_t FileNameW[1024];
    unsigned int Flags;
    unsigned int PackSize;
    unsigned int PackSizeHigh;
    unsigned int UnpSize;
    unsigned int UnpSizeHigh;
    unsigned int HostOS;
    unsigned int FileCRC;
    unsigned int FileTime;
    unsigned int UnpVer;
    unsigned int Method;
    unsigned int FileAttr;
    const char *CmtBuf;
    const wchar_t *CmtBufW;
    unsigned int CmtBufSize;
    unsigned int CmtSize;
    unsigned int CmtState;
    unsigned int Reserved[1024];
    const wchar_t *RedirName;
    unsigned int RedirNameSize;
    unsigned int RedirType;
    const wchar_t *ArcNameEx;
    unsigned int ArcNameExSize;
    const wchar_t *FileNameEx;
    unsigned int FileNameExSize;
    unsigned int Reserved2[4];
} RARHeaderDataEx;

#define UNRAR_DLL_PATH L"external\\UnRAR64.dll"

// Status callback (optional - safe to remove if not used elsewhere)
extern void SendStatus(HWND hwnd, UINT msg, const wchar_t *prefix, const wchar_t *message);
extern void flatten_and_clean_folder(const wchar_t *source, const wchar_t *target);

BOOL extract_cbr(HWND hwnd, const wchar_t *file_path, wchar_t *final_dir);
BOOL extract_unrar_dll(HWND hwnd, const wchar_t *archive_path, const wchar_t *dest_folder);
BOOL create_cbr_archive(HWND hwnd, const wchar_t *image_folder, const wchar_t *archive_name);
