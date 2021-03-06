#ifdef __linux__
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#else
#include <Shlwapi.h>
#include <iphlpapi.h>
#include <Psapi.h>
#include <Tlhelp32.h>
#include "mstring.h"
#include "LogUtil.h"
#include "StrUtil.h"

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Version.lib")
#endif

#include <stdio.h>
#include <string.h>

using namespace std;

void printDbgInternal(const char *tag, const char *file, int line, const char *fmt, ...)
{
    char format1[1024];
    char format2[1024];
    strcpy(format1, "[%hs][%hs.%d]%hs");
    strcat(format1, "\n");
#ifdef __linux__
    snprintf(format2, sizeof(format2), format1, tag, file, line, fmt);
#else
    _snprintf(format2, sizeof(format2), format1, tag, file, line, fmt);
#endif //__linux__

    char logInfo[1024];
    va_list vList;
    va_start(vList, fmt);
    vsnprintf(logInfo, sizeof(logInfo), format2, vList);
    va_end(vList);

#ifndef __linux__
    OutputDebugStringA(logInfo);
#else
    fprintf(stdout, logInfo);
#endif //__linux__
}

#ifndef __linux__
int GetIntFromJson(const Value &json, const std::mstring &name) {
    Value node = json[name];
    if (node.type() == intValue)
    {
        return node.asInt();
    }
    return 0;
}

mstring GetWindowStrA(HWND hwnd) {
    return WtoA(GetWindowStrW(hwnd));
}

std::ustring GetWindowStrW(HWND hwnd) {
    if (!IsWindow(hwnd))
    {
        return L"";
    }

    WCHAR buffer[256];
    buffer[0] = 0;
    int size = GetWindowTextLength(hwnd);
    if (size < 256)
    {
        GetWindowTextW(hwnd, buffer, 256);
        return buffer;
    } else {
        MemoryAlloc<WCHAR> alloc;
        WCHAR *ptr = alloc.GetMemory(size + 4);
        GetWindowTextW(hwnd, ptr, size + 4);
        return ptr;
    }
}

std::mstring GetStrFormJson(const Value &json, const std::mstring &name) {
    Value node = json[name];
    if (node.type() != nullValue)
    {
        if (node.type() == stringValue)
        {
            return node.asString();
        } else {
            return FastWriter().write(node);
        }
    }
    return "";
}

PFILE_MAPPING_STRUCT __stdcall MappingFileA(LPCSTR fileName, BOOL bWrite, DWORD maxViewSize)
{
    PFILE_MAPPING_STRUCT pfms = NULL;

    do
    {
        if (!fileName)
        {
            break;
        }

        pfms = (PFILE_MAPPING_STRUCT)malloc(sizeof(FILE_MAPPING_STRUCT));
        if (!pfms)
        {
            break;
        }
        RtlZeroMemory(pfms, sizeof(FILE_MAPPING_STRUCT));

        pfms->hFile = CreateFileA(
            fileName,
            GENERIC_READ | (bWrite ? GENERIC_WRITE : 0),
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (INVALID_HANDLE_VALUE == pfms->hFile)
        {
            break;
        }

        if (!GetFileSizeEx(pfms->hFile, &(pfms->fileSize)) || !pfms->fileSize.QuadPart)
        {
            break;
        }

        if (!maxViewSize)
        {
            pfms->mappedSize = pfms->fileSize.QuadPart > 0xffffffff ? 0xffffffff : pfms->fileSize.LowPart;
        }
        else
        {
            pfms->mappedSize = pfms->fileSize.QuadPart > maxViewSize ? maxViewSize : pfms->fileSize.LowPart;
        }

        pfms->hMap = CreateFileMapping(pfms->hFile, NULL, bWrite ? PAGE_READWRITE : PAGE_READONLY, 0, 0, NULL);
        if (!pfms->hMap)
        {
            break;
        }

        pfms->lpView = MapViewOfFile(pfms->hMap, FILE_MAP_READ | (bWrite ? FILE_MAP_WRITE : 0), 0, 0, pfms->mappedSize);
        if (!pfms->lpView)
        {
            break;
        }
    } while (FALSE);

    return pfms;
}

void __stdcall CloseFileMapping(PFILE_MAPPING_STRUCT pfms)
{
    if (pfms)
    {
        if (pfms->lpView)
        {
            UnmapViewOfFile(pfms->lpView);
        }

        if (pfms->hMap)
        {
            CloseHandle(pfms->hMap);
        }

        if (INVALID_HANDLE_VALUE != pfms->hFile)
        {
            CloseHandle(pfms->hFile);
        }

        free((void*)pfms);
    }
}

void CentreWindow(HWND hSrcWnd, HWND hDstWnd)
{
    if (!hDstWnd)
    {
        hDstWnd = GetDesktopWindow();
    }

    RECT rt = {0};
    GetWindowRect(hDstWnd, &rt);
    RECT crt = {0};
    GetWindowRect(hSrcWnd, &crt);
    int iX = 0;
    int iY = 0;
    int icW = crt.right - crt.left;
    int iW = rt.right - rt.left;
    int icH = crt.bottom - crt.top;
    int iH = rt.bottom - rt.top;
    iX = rt.left + (iW - icW) / 2;
    iY = rt.top + (iH - icH) / 2;
    MoveWindow(hSrcWnd, iX, iY, icW, icH, TRUE);
}

BOOL GetAdapterSet(OUT vector<AdapterMsg> &nets)
{
    BOOL state = FALSE;
    PIP_ADAPTER_INFO adapterInfo = NULL;
    PIP_ADAPTER_INFO adapter = NULL;
    ULONG length = sizeof(IP_ADAPTER_INFO);
    adapterInfo = (PIP_ADAPTER_INFO)malloc(sizeof(IP_ADAPTER_INFO));
    DWORD ret = GetAdaptersInfo(adapterInfo, &length);
    do 
    {
        if (ERROR_SUCCESS != ret)
        {
            if (ERROR_BUFFER_OVERFLOW == ret)
            {
                adapterInfo = (PIP_ADAPTER_INFO)realloc(adapterInfo, length);
            }
            else
            {
                break;
            }
        }

        if ((GetAdaptersInfo(adapterInfo, &length)) != NO_ERROR)
        {
            break;
        }
        adapter = adapterInfo;
        while (adapter)
        {
            AdapterMsg tmp;
            tmp.m_idex = adapter->ComboIndex;
            tmp.m_name = adapter->AdapterName;
            tmp.m_desc = adapter->Description;
            mstring vt;
            size_t i = 0;
            for (i = 0; i < adapter->AddressLength; i++)
            {
                if (i == (adapter->AddressLength - 1))
                {
                    vt.format("%.2X", (int)adapter->Address[i]);
                }
                else
                {
                    vt.format("%.2X-", (int) adapter->Address[i]);
                }
                tmp.m_mac += vt;
            }
            tmp.m_idex = adapter->Index;
            switch (adapter->Type)
            {
            case MIB_IF_TYPE_OTHER:
                tmp.m_type = "其它";
                break;
            case MIB_IF_TYPE_ETHERNET:
                tmp.m_type = "以太网";
                break;
            case MIB_IF_TYPE_TOKENRING:
                tmp.m_type = "令牌环";
                break;
            case MIB_IF_TYPE_FDDI:
                tmp.m_type = "FDDI";
                break;
            case MIB_IF_TYPE_PPP:
                tmp.m_type = "PPP";
                break;
            case MIB_IF_TYPE_LOOPBACK:
                tmp.m_type = "回路";
                break;
            case MIB_IF_TYPE_SLIP:
                tmp.m_type = "Slip";
                break;
            default:
                tmp.m_type = "未知网卡类型";
                break;
            }
            //ip
            tmp.m_ip = adapter->IpAddressList.IpAddress.String;
            //mask
            tmp.m_mask = adapter->IpAddressList.IpMask.String;
            //网关
            tmp.m_gateway = adapter->GatewayList.IpAddress.String;

            if (adapter->DhcpEnabled)
            {
                tmp.m_dhcp_enable = true;
            }
            else
            {
                tmp.m_dhcp_enable = false;
            }
            adapter = adapter->Next;
            nets.push_back(tmp);
            state = TRUE;
        }
    } while (FALSE);

    if (adapterInfo)
    {
        free(adapterInfo);
    }
    return state;
}

BOOL EnumFiles(const mstring &dir, BOOL recursion, pfnFileHandler handler, void *param) {
    if (dir.empty() || !handler)
    {
        return FALSE;
    }

    DWORD attrib = GetFileAttributesA(dir.c_str());
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        return FALSE;
    }

    list<mstring> dirSet;
    dirSet.push_back(dir);
    char findStr[MAX_PATH];
    mstring curDir;
    bool exit = false;
    while (!dirSet.empty()) {
        curDir = *dirSet.begin();
        dirSet.pop_front();

        lstrcpyA(findStr, curDir.c_str());
        PathAppendA(findStr, "*");

        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(findStr, &findData);
        if (INVALID_HANDLE_VALUE == hFind) {
            continue;
        }

        BOOL bRet = TRUE;
        do
        {
            if (0 == lstrcmpA(findData.cFileName, ".") || 0 == lstrcmpA(findData.cFileName, ".."))
            {
                continue;
            }

            char fileName[MAX_PATH];
            lstrcpyA(fileName, curDir.c_str());
            PathAppendA(fileName, findData.cFileName);

            if (INVALID_FILE_ATTRIBUTES == findData.dwFileAttributes)
            {
                continue;
            }

            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (recursion)
                {
                    dirSet.push_back(fileName);
                }

                if (!handler(true, fileName, param))
                {
                    exit = true;
                    break;
                }
            }
            else
            {
                if (!handler(false, fileName, param))
                {
                    exit = true;
                    break;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);

        if (true == exit)
        {
            break;
        }
    }
    return TRUE;
}

static BOOL _GetNtVersionNumbers(DWORD&dwMajorVer, DWORD& dwMinorVer, DWORD& dwBuildNumber)
{
    BOOL bRet = FALSE;
    HMODULE hModNtdll = GetModuleHandleA("ntdll.dll");
    if (hModNtdll)
    {
        typedef void (WINAPI* RtlGetNtVersionNumbers)(DWORD*, DWORD*, DWORD*);
        RtlGetNtVersionNumbers pRtlGetNtVersionNumbers = (RtlGetNtVersionNumbers)GetProcAddress(hModNtdll, "RtlGetNtVersionNumbers");
        if (pRtlGetNtVersionNumbers)
        {
            pRtlGetNtVersionNumbers(&dwMajorVer, &dwMinorVer, &dwBuildNumber);
            dwBuildNumber &= 0x0ffff;
            bRet = TRUE;
        }
    }

    return bRet;
}

mstring GetOSVersion()
{
    mstring ver;
    OSVERSIONINFOEXA osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&osvi);

    if (!_GetNtVersionNumbers(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
    {
        return "UnKnown";
    }

    // 不支持非NT的系统
    if (VER_PLATFORM_WIN32_NT != osvi.dwPlatformId)
    {
        return "UnKnown";
    }

    if (osvi.dwMajorVersion <= 4)
    {
        ver += "Microsoft Windows NT ";
    }
    else if (5 == osvi.dwMajorVersion)
    {
        switch (osvi.dwMinorVersion) {
            case 0:
                ver += "Microsoft Windows 2000 ";
                break;
            case 1:
                ver += "Microsoft Windows XP ";
                break;
            case 2:
                ver += "Microsoft Server 2003 ";
                break;
        }
    }
    else if (6 == osvi.dwMajorVersion)
    {
        if (0 == osvi.dwMinorVersion)
        {
            if (osvi.wProductType == VER_NT_WORKSTATION)
            {
                ver += "Microsoft Windows Vista ";
            }
            else
            {
                ver += "Microsoft Server 2008 ";
            }
        }
        else if (1 == osvi.dwMinorVersion)
        {
            if (osvi.wProductType == VER_NT_WORKSTATION)
            {
                ver += "Microsoft Windows 7 ";
            }
            else {
                ver += "Microsoft Windows Server 2008 R2 ";
            }
        }
        else if (2 == osvi.dwMinorVersion)
        {
            if (osvi.wProductType == VER_NT_WORKSTATION)
            {
                ver += "Microsoft Windows 8 ";
            }
        }
        else if (3 == osvi.dwMinorVersion)
        {
            if (osvi.wProductType == VER_NT_WORKSTATION)
            {
                ver += "Microsoft Windows 8.1 ";
            }
        }
    }
    else if (10 == osvi.dwMajorVersion)
    {
        if (0 == osvi.dwMinorVersion)
        {
            if (osvi.wProductType == VER_NT_WORKSTATION)
            {
                ver += "Microsoft Windows 10 ";
            }
        }
    }

    char temp[256];
    if (osvi.dwMajorVersion <= 4)
    {
        wnsprintfA(
            temp,
            256,
            "version %d.%d %s (Build %d)",
            osvi.dwMajorVersion,
            osvi.dwMinorVersion,
            osvi.szCSDVersion,
            osvi.dwBuildNumber & 0xFFFF
            );
        ver += temp;
    }
    else
    {
        wnsprintfA(
            temp,
            256,
            "%s (Build %d)",
            osvi.szCSDVersion,
            osvi.dwBuildNumber & 0xFFFF
            );
        ver += temp;
    }
    return ver;
}

static BOOL _FileDosNameToFileName(LPCSTR dosName, LPSTR fileName, UINT bufSize)
{
    BOOL bRet = FALSE;

    do
    {
        if (!dosName || !fileName)
        {
            break;
        }
        if (lstrlenA(dosName) > 250)
        {
            break;
        }

        if (dosName[0] != '\\')
        {
            break;
        }

        LPCSTR secondBackSlash = StrChrA(dosName + 1, '\\');
        if (!secondBackSlash)
        {
            break;
        }

        LPCSTR thirdBackSlash = StrChrA(secondBackSlash + 1, '\\');
        if (!thirdBackSlash)
        {
            break;
        }

        DWORD drives = GetLogicalDrives();
        for (int i = 0; i < 26; ++i)
        {
            if (!(drives & (1 << i)))
            {
                continue;
            }

            char letter[4] = {'c', ':'};
            letter[0] = 'A' + i;
            char dosDevice[MAX_PATH];

            if (QueryDosDeviceA(letter, dosDevice, MAX_PATH))
            {
                if (0 == StrCmpNIA(dosDevice, dosName, (int)(thirdBackSlash - dosName)))
                {
                    wnsprintfA(fileName, bufSize, "%s%s", letter, thirdBackSlash);
                    bRet = TRUE;
                    break;
                }
            }
        }
    } while (FALSE);

    return bRet;
}

mstring GetProcPathFromPid(DWORD pid) {
    HANDLE hProc;
    DWORD dwCopied = 0;
    DWORD dwLastError = 0;
    char buff[256];
    buff[0] = 0x00;
    DWORD buffSize = 256;
    mstring result;

    do
    {
        hProc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hProc == NULL)
        {
            dwLastError = GetLastError();
            break;
        }

        dwCopied = GetModuleFileNameExA(hProc, NULL, buff, 256);
        if (dwCopied > 0)
        {
            dwLastError = GetLastError();
            result = buff;
            break;
        }
        //兼容64 & 2k
        if (dwCopied == 0)
        {
            char psapi[MAX_PATH];
            GetSystemDirectoryA(psapi, MAX_PATH);
            PathAppendA(psapi, "psapi.dll");
            HMODULE hMod = LoadLibraryA(psapi);
            if (hMod == NULL)
            {
                dwLastError = GetLastError();
                break;
            }

            typedef DWORD (WINAPI *PGetProcessImageFileNameW)(HANDLE, LPSTR, DWORD);
            PGetProcessImageFileNameW pFun = (PGetProcessImageFileNameW)GetProcAddress(hMod, "GetProcessImageFileNameA");
            if (pFun == NULL)
            {
                dwLastError = GetLastError();
                FreeLibrary(hMod);
                break;
            }

            dwCopied = pFun(hProc, buff, buffSize);
            if (dwCopied > 0)
            {
                dwCopied = _FileDosNameToFileName(buff, buff, buffSize);
                dwLastError = GetLastError();
            }
            FreeLibrary(hMod);
        }
    } while (FALSE);

    CloseHandle(hProc);
    SetLastError(dwLastError);
    return result;
}

typedef BOOL (WINAPI* pfnProcHandler)(PPROCESSENTRY32 info, void *param);

void ProcIterateProc(pfnProcHandler handler, void* lpParam)
{
    do
    {
        if (!handler)
        {
            break;
        }

        PROCESSENTRY32 pe32 = {0};
        pe32.dwSize = sizeof(pe32);

        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (INVALID_HANDLE_VALUE == hSnap)
        {
            break;
        }

        BOOL bMore = Process32First(hSnap, &pe32);
        while (bMore)
        {
            if (!handler(&pe32, lpParam))
            {
                // 回调方要求退出
                break;
            }

            bMore = Process32Next(hSnap, &pe32);
        }

        CloseHandle(hSnap);
    } while (FALSE);
}

BOOL GetFileVersion(LPCSTR pFile, mstring& ver)
{   
    LPSTR pBuf = NULL;
    CHAR szVersionBuffer[4096] = {0x00};
    DWORD dwVerSize = 0;
    DWORD dwHandle = 0;
    DWORD dwLength = 4096;
    BOOL bState = FALSE;

    dwVerSize = GetFileVersionInfoSizeA(pFile, &dwHandle);
    if (dwVerSize > dwLength)
    {
        pBuf = new CHAR[dwVerSize];
        dwLength = dwVerSize;
    }
    else
    {
        pBuf = szVersionBuffer;
    }

    if (GetFileVersionInfoA(pFile, 0, dwLength, pBuf))   
    {   
        VS_FIXEDFILEINFO * pInfo = NULL;
        unsigned int nInfoLen = 0;   
        if (VerQueryValueA(pBuf, "\\", (void**)&pInfo, &nInfoLen))  
        {  
            wnsprintfA(pBuf, dwLength, "%d.%d.%d.%d", HIWORD(pInfo->dwFileVersionMS), LOWORD(pInfo->dwFileVersionMS), HIWORD(pInfo->dwFileVersionLS), LOWORD(pInfo->dwFileVersionLS));
            ver = szVersionBuffer;
            bState = TRUE;
        }
    }

    if (pBuf != szVersionBuffer)
    {
        delete []pBuf;
    }
    return bState;
}

static BOOL _WriteFileBuffer(HANDLE file, const char *buffer, size_t length)
{
    DWORD write = 0;
    DWORD ms = 0;
    while(write < length)
    {
        ms = 0;
        if (!WriteFile(file, buffer + write, length - write, &ms, NULL))
        {
            return FALSE;
        }
        write += ms;
    }
    return TRUE;
}

//从PE的资源中释放文件
BOOL ReleaseRes(const char *path, DWORD id, const char *type)
{
    HRSRC src = NULL;
    HGLOBAL hg = NULL;
    HANDLE file = INVALID_HANDLE_VALUE;
    BOOL state = FALSE;
    do 
    {
        src = FindResourceA(NULL, MAKEINTRESOURCEA(id), type);
        if (!src)
        {
            break;
        }

        hg = LoadResource(NULL, src);
        if (!hg)
        {
            break;
        }

        DWORD size = SizeofResource(NULL, src);  
        file = CreateFileA(
            path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            0,
            NULL
            );
        if (!file || INVALID_HANDLE_VALUE == file)
        {
            break;
        }

        if (size <= 0)
        {
            break;
        }

        if(!_WriteFileBuffer(file, (const char *)hg, size))
        {
            break;
        }
        state = TRUE;
    } while (FALSE);

    if(hg)
    {
        FreeResource(hg);
    }

    if (file && INVALID_HANDLE_VALUE != file)
    {
        CloseHandle(file);
    }
    return TRUE;  
}
#endif //__linux__