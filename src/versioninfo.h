// versioninfo.h
#ifndef VERSIONINFO_H
#define VERSIONINFO_H

typedef struct {
    wchar_t CompanyName[64];
    wchar_t FileDescription[64];
    wchar_t FileVersion[32];
    wchar_t InternalName[64];
    wchar_t OriginalFilename[64];
    wchar_t ProductName[64];
    wchar_t ProductVersion[32];
    wchar_t LegalCopyright[128];
} AppVersionInfo;

void GetAppVersionFields(AppVersionInfo *info);

#endif