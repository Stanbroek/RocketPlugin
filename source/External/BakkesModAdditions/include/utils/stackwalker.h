/**********************************************************************
 *
 * StackWalker.h
 * https://github.com/JochenKalmbach/StackWalker
 *
 * Old location: http://stackwalker.codeplex.com/
 *
 *
 * LICENSE (http://www.opensource.org/licenses/bsd-license.php)
 *
 *   Copyright (c) 2005-2009, Jochen Kalmbach
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without modification,
 *   are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *   Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *   Neither the name of Jochen Kalmbach nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * **********************************************************************/
#pragma once
#if defined(UNICODE) && !defined(DBGHELP_TRANSLATE_TCHAR)
    #define DBGHELP_TRANSLATE_TCHAR

    #define strcpy_s    wcscpy_s
    #define strncpy_s   wcsncpy_s
    #define _strdup     _wcsdup
    #define strcat_s    wcscat_s
    #define strlen      wcslen
    #define strrchr     wcsrchr
#endif

#include <Windows.h>
#include <cstdlib>
#include <tchar.h>
#pragma comment(lib, "version.lib")

#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#include <TlHelp32.h>
#pragma comment(lib, "Kernel32.lib")
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

#include "utils/stringify.h"

#ifndef FMT_HEADER_ONLY
    #define FMT_HEADER_ONLY
#endif
#include "fmt/format.h"
#include "fmt/xchar.h"
#include "fmt/chrono.h"

#define USED_CONTEXT_FLAGS      CONTEXT_FULL
#define STACKWALK_MAX_NAMELEN   1024
#define GET_CURRENT_CONTEXT_STACKWALKER_CODEPLEX(c, contextFlags)   \
    do {                                                            \
        memset(&(c), 0, sizeof CONTEXT);                            \
        (c).ContextFlags = contextFlags;                            \
        RtlCaptureContext(&(c));                                    \
    } while (0);

#if defined(UNICODE)
static BOOL SymGetSymFromAddrW64(
    _In_ HANDLE hProcess,
    _In_ DWORD64 qwAddr,
    _Out_opt_ PDWORD64 pdwDisplacement,
    _Inout_ PIMAGEHLP_SYMBOLW64  Symbol
)
{
    const SIZE_T       symSize = sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN;
    PIMAGEHLP_SYMBOL64 pSym = (PIMAGEHLP_SYMBOL64)malloc(symSize);
    ZeroMemory(pSym, symSize);
    pSym->SizeOfStruct  = Symbol->SizeOfStruct;
    pSym->Address       = Symbol->Address;
    pSym->Size          = Symbol->Size;
    pSym->Flags         = Symbol->Flags;
    pSym->MaxNameLength = Symbol->MaxNameLength;

    BOOL ret = SymGetSymFromAddr64(hProcess, qwAddr, pdwDisplacement, pSym);
    Symbol->SizeOfStruct    = pSym->SizeOfStruct;
    Symbol->Address         = pSym->Address;
    Symbol->Size            = pSym->Size;
    Symbol->Flags           = pSym->Flags;
    Symbol->MaxNameLength   = pSym->MaxNameLength;
    MultiByteToWideChar(CP_UTF8, NULL, pSym->Name, pSym->MaxNameLength, Symbol->Name, Symbol->MaxNameLength);

    return ret;
}

#define IMAGEHLP_SYMBOL64  IMAGEHLP_SYMBOLW64
#define PIMAGEHLP_SYMBOL64  PIMAGEHLP_SYMBOLW64
#define SymGetSymFromAddr64  SymGetSymFromAddrW64
#endif

extern std::filesystem::path BakkesModCrashesFolder;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;


class StackWalker
{
public:
    typedef enum StackWalkOptions
    {
        // No addition info will be retrieved
        // (only the address is available)
        RetrieveNone = 0,

        // Try to get the symbol-name
        RetrieveSymbol = 1,

        // Try to get the line for this symbol
        RetrieveLine = 2,

        // Try to retrieve the module-infos
        RetrieveModuleInfo = 4,

        // Also retrieve the version for the DLL/EXE
        RetrieveFileVersion = 8,

        // Contains all the above
        RetrieveVerbose = 0xF,

        // Generate a "good" symbol-search-path
        SymBuildPath = 0x10,

        // Also use the public Microsoft-Symbol-Server
        SymUseSymSrv = 0x20,

        // Contains all the above "Sym"-options
        SymAll = 0x30,

        // Contains all options (default)
        OptionsAll = 0x3F
    } StackWalkOptions;

    StackWalker(int options = OptionsAll, // 'int' is by design, to combine the enum-flags
        LPCTSTR szSymPath = NULL,
        DWORD  dwProcessId = GetCurrentProcessId(),
        HANDLE hProcess = GetCurrentProcess())
    {
        m_options = options;
        m_modulesLoaded = FALSE;
        m_hProcess = hProcess;
        m_dwProcessId = dwProcessId;
        if (szSymPath != NULL) {
            m_szSymPath = _strdup(szSymPath);
            m_options |= SymBuildPath;
        }
        else
            m_szSymPath = NULL;
        m_MaxRecursionCount = 1000;
    }
    StackWalker(DWORD dwProcessId, HANDLE hProcess)
    {
        m_options = OptionsAll;
        m_modulesLoaded = FALSE;
        m_hProcess = hProcess;
        m_dwProcessId = dwProcessId;
        m_szSymPath = NULL;
        m_MaxRecursionCount = 1000;
    }
    virtual ~StackWalker()
    {
        if (m_szSymPath != NULL)
            free(m_szSymPath);
        m_szSymPath = NULL;
        SymCleanup(m_hProcess);
    }

    BOOL LoadModules()
    {
        if (m_modulesLoaded != FALSE)
            return TRUE;

        // Build the sym-path:
        PTCHAR szSymPath = NULL;
        if ((m_options & SymBuildPath) != 0) {
            const size_t nSymPathLen = 4096;
            szSymPath = (PTCHAR)malloc(nSymPathLen);
            if (szSymPath == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            szSymPath[0] = 0;
            // Now first add the (optional) provided sympath:
            if (m_szSymPath != NULL) {
                strcat_s(szSymPath, nSymPathLen, m_szSymPath);
                strcat_s(szSymPath, nSymPathLen, TEXT(";"));
            }

            strcat_s(szSymPath, nSymPathLen, TEXT(".;"));

            const size_t nTempLen = 1024;
            TCHAR        szTemp[nTempLen];
            // Now add the current directory:
            if (GetCurrentDirectory(nTempLen, szTemp) > 0) {
                szTemp[nTempLen - 1] = 0;
                strcat_s(szSymPath, nSymPathLen, szTemp);
                strcat_s(szSymPath, nSymPathLen, TEXT(";"));
            }

            // Now add the path for the main-module:
            if (GetModuleFileName(NULL, szTemp, nTempLen) > 0) {
                szTemp[nTempLen - 1] = 0;
                for (PTCHAR p = (szTemp + strlen(szTemp) - 1); p >= szTemp; --p) {
                    // locate the rightmost path separator
                    if ((*p == '\\') || (*p == '/') || (*p == ':')) {
                        *p = 0;
                        break;
                    }
                } // for (search for path separator...)
                if (strlen(szTemp) > 0) {
                    strcat_s(szSymPath, nSymPathLen, szTemp);
                    strcat_s(szSymPath, nSymPathLen, TEXT(";"));
                }
            }
            if (GetEnvironmentVariable(TEXT("_NT_SYMBOL_PATH"), szTemp, nTempLen) > 0) {
                szTemp[nTempLen - 1] = 0;
                strcat_s(szSymPath, nSymPathLen, szTemp);
                strcat_s(szSymPath, nSymPathLen, TEXT(";"));
            }
            if (GetEnvironmentVariable(TEXT("_NT_ALTERNATE_SYMBOL_PATH"), szTemp, nTempLen) > 0) {
                szTemp[nTempLen - 1] = 0;
                strcat_s(szSymPath, nSymPathLen, szTemp);
                strcat_s(szSymPath, nSymPathLen, TEXT(";"));
            }
            if (GetEnvironmentVariable(TEXT("SYSTEMROOT"), szTemp, nTempLen) > 0) {
                szTemp[nTempLen - 1] = 0;
                strcat_s(szSymPath, nSymPathLen, szTemp);
                strcat_s(szSymPath, nSymPathLen, TEXT(";"));
                // also add the "system32"-directory:
                strcat_s(szTemp, nTempLen, TEXT("\\system32"));
                strcat_s(szSymPath, nSymPathLen, szTemp);
                strcat_s(szSymPath, nSymPathLen, TEXT(";"));
            }

            if ((m_options & SymUseSymSrv) != 0) {
                if (GetEnvironmentVariable(TEXT("SYSTEMDRIVE"), szTemp, nTempLen) > 0) {
                    szTemp[nTempLen - 1] = 0;
                    strcat_s(szSymPath, nSymPathLen, TEXT("SRV*"));
                    strcat_s(szSymPath, nSymPathLen, szTemp);
                    strcat_s(szSymPath, nSymPathLen, TEXT("\\websymbols"));
                    strcat_s(szSymPath, nSymPathLen, TEXT("*https://msdl.microsoft.com/download/symbols;"));
                }
                else
                    strcat_s(szSymPath, nSymPathLen, TEXT("SRV*c:\\websymbols*https://msdl.microsoft.com/download/symbols;"));
            }
        } // if SymBuildPath

        // First Init the whole stuff...
        Init(szSymPath);

        BOOL bRet = LoadModules(m_hProcess, m_dwProcessId);
        if (bRet != FALSE)
            m_modulesLoaded = TRUE;

        return bRet;
    }

    BOOL ShowCallStack(HANDLE hThread = GetCurrentThread(),
        const CONTEXT* context = NULL) noexcept
    {
        CONTEXT            c;
        CallStackEntry     csEntry;
        IMAGEHLP_SYMBOL64* pSym = NULL;
        IMAGEHLP_MODULE64  Module;
        IMAGEHLP_LINE64    Line;
        int                frameNum;
        bool               bLastEntryCalled = true;
        int                curRecursionCount = 0;

        if (m_modulesLoaded == FALSE)
            LoadModules();
        
        if (context == NULL) {
            // If no context is provided, capture the context
            // See: https://stackwalker.codeplex.com/discussions/446958
            if (GetThreadId(hThread) == GetCurrentThreadId()) {
                GET_CURRENT_CONTEXT_STACKWALKER_CODEPLEX(c, USED_CONTEXT_FLAGS);
            }
            else {
                SuspendThread(hThread);
                memset(&c, 0, sizeof(CONTEXT));
                c.ContextFlags = USED_CONTEXT_FLAGS;

                // TODO: Detect if you want to get a thread context of a different process, which is running a different processor architecture...
                // This does only work if we are x64 and the target process is x64 or x86;
                // It cannot work, if this process is x64 and the target process is x64... this is not supported...
                // See also: http://www.howzatt.demon.co.uk/articles/DebuggingInWin64.html
                if (GetThreadContext(hThread, &c) == FALSE) {
                    ResumeThread(hThread);
                    return FALSE;
                }
            }
        }
        else
            c = *context;

        // init STACKFRAME for first call
        STACKFRAME64 s; // in/out stackframe
        memset(&s, 0, sizeof(s));
        DWORD imageType;
        imageType = IMAGE_FILE_MACHINE_AMD64;
        s.AddrPC.Offset = c.Rip;
        s.AddrPC.Mode = AddrModeFlat;
        s.AddrFrame.Offset = c.Rsp;
        s.AddrFrame.Mode = AddrModeFlat;
        s.AddrStack.Offset = c.Rsp;
        s.AddrStack.Mode = AddrModeFlat;

        pSym = (IMAGEHLP_SYMBOL64*)malloc(sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN);
        if (!pSym)
            goto cleanup; // not enough memory...
        memset(pSym, 0, sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN);
        pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        pSym->MaxNameLength = STACKWALK_MAX_NAMELEN;

        memset(&Line, 0, sizeof(Line));
        Line.SizeOfStruct = sizeof(Line);

        memset(&Module, 0, sizeof(Module));
        Module.SizeOfStruct = sizeof(Module);

        for (frameNum = 0;; ++frameNum) {
            // get next stack frame if this returns ERROR_INVALID_ADDRESS (487) or
            // ERROR_NOACCESS (998), you can assume that either you are done, or
            // that the stack is so hosed that the next deeper frame could not be found.
            // CONTEXT need not to be supplied if imageTyp is IMAGE_FILE_MACHINE_I386!
            if (!StackWalk64(imageType, m_hProcess, hThread, &s, &c, myReadProcMem,
                SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
                // INFO: "StackWalk64" does not set "GetLastError"...
                BM_DEBUG_ERROR_LOG("StackWalk64, GetLastError: {:X} (Address: {:p})", 0, (LPVOID)s.AddrPC.Offset);
                break;
            }

            csEntry.offset = s.AddrPC.Offset;
            csEntry.name[0] = 0;
            csEntry.undName[0] = 0;
            csEntry.undFullName[0] = 0;
            csEntry.offsetFromSmybol = 0;
            csEntry.offsetFromLine = 0;
            csEntry.lineFileName[0] = 0;
            csEntry.lineNumber = 0;
            csEntry.loadedImageName[0] = 0;
            csEntry.moduleName[0] = 0;
            if (s.AddrPC.Offset == s.AddrReturn.Offset) {
                if ((m_MaxRecursionCount > 0) && (curRecursionCount > m_MaxRecursionCount)) {
                    BM_DEBUG_ERROR_LOG("StackWalk64-Endless-Callstack!, GetLastError: {:X} (Address: {:p})", 0, (LPVOID)s.AddrPC.Offset);
                    break;
                }
                curRecursionCount++;
            }
            else
                curRecursionCount = 0;
            if (s.AddrPC.Offset != 0) {
                // we seem to have a valid PC
                // show procedure info
                if (SymGetSymFromAddr64(m_hProcess, s.AddrPC.Offset, &(csEntry.offsetFromSmybol), pSym) != FALSE) {
                    MyStrCpy(csEntry.name, STACKWALK_MAX_NAMELEN, pSym->Name);
                    UnDecorateSymbolName(pSym->Name, csEntry.undName, STACKWALK_MAX_NAMELEN, UNDNAME_NAME_ONLY);
                    UnDecorateSymbolName(pSym->Name, csEntry.undFullName, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE);
                }
                else {
                    BM_DEBUG_ERROR_LOG("SymGetSymFromAddr64, GetLastError: {:X} (Address: {:p})", GetLastError(), (LPVOID)s.AddrPC.Offset);
                }

                // show line number info, NT5.0-method
                if (SymGetLineFromAddr64(m_hProcess, s.AddrPC.Offset, &(csEntry.offsetFromLine),
                    &Line) != FALSE) {
                    csEntry.lineNumber = Line.LineNumber;
                    MyStrCpy(csEntry.lineFileName, STACKWALK_MAX_NAMELEN, Line.FileName);
                }
                else {
                    BM_DEBUG_ERROR_LOG("SymGetLineFromAddr64, GetLastError: {:X} (Address: {:p})", GetLastError(), (LPVOID)s.AddrPC.Offset);
                }

                // show module info
                if (GetModuleInfo(m_hProcess, s.AddrPC.Offset, &Module) != FALSE) {
                    // got module info OK
                    switch (Module.SymType) {
                        case SymNone:
                            csEntry.symTypeString = TEXT("-nosymbols-");
                            break;
                        case SymCoff:
                            csEntry.symTypeString = TEXT("COFF");
                            break;
                        case SymCv:
                            csEntry.symTypeString = TEXT("CV");
                            break;
                        case SymPdb:
                            csEntry.symTypeString = TEXT("PDB");
                            break;
                        case SymExport:
                            csEntry.symTypeString = TEXT("-exported-");
                            break;
                        case SymDeferred:
                            csEntry.symTypeString = TEXT("-deferred-");
                            break;
                        case SymSym:
                            csEntry.symTypeString = TEXT("SYM");
                            break;
                        case SymDia:
                            csEntry.symTypeString = TEXT("DIA");
                            break;
                        case 8: //SymVirtual:
                            csEntry.symTypeString = TEXT("Virtual");
                            break;
                        default:
                            BM_DEBUG_TRACE_LOG("symtype={:ld}", Module.SymType);
                            csEntry.symTypeString = NULL;
                            break;
                    }

                    MyStrCpy(csEntry.moduleName, STACKWALK_MAX_NAMELEN, Module.ModuleName);
                    csEntry.baseOfImage = Module.BaseOfImage;
                    MyStrCpy(csEntry.loadedImageName, STACKWALK_MAX_NAMELEN, Module.LoadedImageName);
                } // got module info OK
                else
                    BM_DEBUG_ERROR_LOG("SymGetModuleInfo64, GetLastError: {:X} (Address: {:p})", GetLastError(), (LPVOID)s.AddrPC.Offset);
            } // we seem to have a valid PC

            CallStackEntryType et = nextEntry;
            if (frameNum == 0)
                et = firstEntry;
            bLastEntryCalled = false;
            OnCallStackEntry(et, csEntry);

            if (s.AddrReturn.Offset == 0) {
                bLastEntryCalled = true;
                OnCallStackEntry(lastEntry, csEntry);
                SetLastError(ERROR_SUCCESS);
                break;
            }
        } // for ( frameNum )

    cleanup:
        if (pSym)
            free(pSym);

        if (bLastEntryCalled == false)
            OnCallStackEntry(lastEntry, csEntry);

        if (context == NULL)
            ResumeThread(hThread);

        return TRUE;
    }

    BOOL ShowObject(LPVOID pObject)
    {
        // Show object info
        DWORD64            dwAddress = DWORD64(pObject);
        DWORD64            dwDisplacement = 0;
        const SIZE_T       symSize = sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN;
        IMAGEHLP_SYMBOL64* pSym = (IMAGEHLP_SYMBOL64*)malloc(symSize);
        if (!pSym)
            return FALSE;
        memset(pSym, 0, symSize);
        pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        pSym->MaxNameLength = STACKWALK_MAX_NAMELEN;
        if (SymGetSymFromAddr64(m_hProcess, dwAddress, &dwDisplacement, pSym) == FALSE) {
            BM_DEBUG_ERROR_LOG("SymGetSymFromAddr64, GetLastError: {:X} (Address: {:p})", GetLastError(), (LPVOID)dwAddress);
            return FALSE;
        }
        // Object name output
        OnOutput(pSym->Name);

        free(pSym);
        return TRUE;
    }

protected:
    // Entry for each CallStack-Entry
    typedef struct CallStackEntry
    {
        DWORD64 offset; // if 0, we have no valid entry
        TCHAR    name[STACKWALK_MAX_NAMELEN];
        TCHAR    undName[STACKWALK_MAX_NAMELEN];
        TCHAR    undFullName[STACKWALK_MAX_NAMELEN];
        DWORD64 offsetFromSmybol;
        DWORD   offsetFromLine;
        DWORD   lineNumber;
        TCHAR    lineFileName[STACKWALK_MAX_NAMELEN];
        DWORD   symType;
        LPCTSTR  symTypeString;
        TCHAR    moduleName[STACKWALK_MAX_NAMELEN];
        DWORD64 baseOfImage;
        TCHAR    loadedImageName[STACKWALK_MAX_NAMELEN];
    } CallStackEntry;

    typedef enum CallStackEntryType
    {
        firstEntry,
        nextEntry,
        lastEntry
    } CallStackEntryType;

    virtual void OnSymInit(LPCTSTR szSearchPath, DWORD symOptions, LPCTSTR szUserName)
    {
        auto buffer = fmt::format(TEXT("SymInit: Symbol-SearchPath: '{:s}', symOptions: {:d}, UserName: '{:s}'"),
            szSearchPath, symOptions, szUserName);
        OnOutput(buffer.c_str());
        // Getting the OS-version is deprecated.
        //OSVERSIONINFOEXA ver;
        //ZeroMemory(&ver, sizeof(OSVERSIONINFOEXA));
        //ver.dwOSVersionInfoSize = sizeof(ver);
        //if (GetVersionExA((OSVERSIONINFOA*)&ver) != FALSE) {
        //	buffer = fmt::format("OS-Version: {:d}.{:d}.{:d} ({:s}) {:X}-{:X}",
        //		ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber, ver.szCSDVersion,
        //		ver.wSuiteMask, ver.wProductType);
        //	OnOutput(buffer.c_str());
        //}
    }

    virtual void OnLoadModule(LPCTSTR img,
        LPCTSTR    mod,
        DWORD64   baseAddr,
        DWORD     size,
        DWORD     result,
        LPCTSTR    symType,
        LPCTSTR    pdbName,
        ULONGLONG fileVersion)
    {
        std::basic_string<TCHAR> buffer;

        if (fileVersion == 0)
            buffer = fmt::format(TEXT("{:s}:{:s} ({:p}), size: {:d} (result: {:d}), SymType: '{:s}', PDB: '{:s}'"),
                img, mod, (LPVOID)baseAddr, size, result, symType, pdbName);
        else {
            DWORD v4 = (DWORD)(fileVersion & 0xFFFF);
            DWORD v3 = (DWORD)((fileVersion >> 16) & 0xFFFF);
            DWORD v2 = (DWORD)((fileVersion >> 32) & 0xFFFF);
            DWORD v1 = (DWORD)((fileVersion >> 48) & 0xFFFF);
            buffer = fmt::format(TEXT("{:s}:{:s} ({:p}), size: {:d} (result: {:d}), SymType: '{:s}', PDB: '{:s}', fileVersion: {:d}.{:d}.{:d}.{:d}"),
                img, mod, (LPVOID)baseAddr, size, result, symType, pdbName, v1, v2, v3, v4);
        }

        OnOutput(buffer.c_str());
    }

    virtual void OnCallStackEntry(CallStackEntryType eType, CallStackEntry& entry)
    {
        if (eType != lastEntry && entry.offset != 0) {
            std::basic_string<TCHAR> function = fmt::format(TEXT("0x{:X}"), entry.offset);
            if (entry.undFullName[0] != TEXT('\0'))
                function = fmt::format(TEXT("{:s} {:s}()"), function, entry.undFullName);
            else if (entry.undName[0] != TEXT('\0'))
                function = fmt::format(TEXT("{:s} {:s}()"), function, entry.undName);
            else if (entry.name[0] != TEXT('\0'))
                function = fmt::format(TEXT("{:s} {:s}()"), function, entry.name);
            std::basic_string<TCHAR> filename = TEXT("(filename not available)");
            if (entry.lineFileName[0] != TEXT('\0'))
                filename = fmt::format(TEXT("[File={:s}:{:d}]"), entry.lineFileName, entry.lineNumber);
            std::basic_string<TCHAR> moduleName = TEXT("(module-name not available)");
            if (entry.moduleName[0] != TEXT('\0'))
                moduleName = fmt::format(TEXT("[in {:s}]"), entry.moduleName);

            OnOutput(fmt::format(TEXT("{:s} {:s} {:s}"), function, filename, moduleName).c_str());
        }
    }

    virtual void OnOutput([[maybe_unused]] LPCTSTR szText)
    {
        BM_DEBUG_INFO_LOG(to_string(szText));
    }

    HANDLE               m_hProcess;
    DWORD                m_dwProcessId;
    BOOL                 m_modulesLoaded;
    LPTSTR                m_szSymPath;

    int m_options;
    int m_MaxRecursionCount;

    static BOOL __stdcall myReadProcMem(HANDLE  hProcess,
        DWORD64 qwBaseAddress,
        PVOID   lpBuffer,
        DWORD   nSize,
        LPDWORD lpNumberOfBytesRead)
    {
        SIZE_T st;
        BOOL   bRet = ReadProcessMemory(hProcess, (LPVOID)qwBaseAddress, lpBuffer, nSize, &st);
        *lpNumberOfBytesRead = (DWORD)st;
        //__BM_TRACE_LOG("ReadMemory: hProcess: {:p}, baseAddr: {:p}, buffer: {:p}, size: {:d}, read: {:d}, result: {:d}", hProcess, (LPVOID)qwBaseAddress, lpBuffer, nSize, (DWORD)st, (DWORD)bRet);

        return bRet;
    }

private:
    static void MyStrCpy(PTCHAR szDest, size_t nMaxDestSize, const TCHAR* szSrc)
    {
        if (nMaxDestSize <= 0)
            return;
        strncpy_s(szDest, nMaxDestSize, szSrc, _TRUNCATE);
        // INFO: _TRUNCATE will ensure that it is null-terminated;
        // but with older compilers (<1400) it uses "strncpy" and this does not!)
        szDest[nMaxDestSize - 1] = 0;
    }

    BOOL Init(LPCTSTR szSymPath)
    {
        // SymInitialize
        if (szSymPath != NULL)
            szSymPath = _strdup(szSymPath);
        if (SymInitialize(m_hProcess, szSymPath, FALSE) == FALSE)
            BM_DEBUG_ERROR_LOG("SymInitialize, GetLastError: {:X} (Address: {:p})", GetLastError(), (LPVOID)0);

        DWORD symOptions = SymGetOptions();
        symOptions |= SYMOPT_LOAD_LINES;
        symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
        //symOptions |= SYMOPT_NO_PROMPTS;
        symOptions = SymSetOptions(symOptions);

        TCHAR buf[STACKWALK_MAX_NAMELEN] = { 0 };
        if (SymGetSearchPath(m_hProcess, buf, STACKWALK_MAX_NAMELEN) == FALSE)
            BM_DEBUG_ERROR_LOG("SymGetSearchPath, GetLastError: {:X} (Address: {:p})", GetLastError(), (LPVOID)0);
        TCHAR szUserName[1024] = { 0 };
        DWORD dwSize = 1024;
        GetUserName(szUserName, &dwSize);
        OnSymInit(buf, symOptions, szUserName);

        return TRUE;
    }

    BOOL GetModuleListTH32(HANDLE hProcess, DWORD pid)
    {
        HANDLE hSnap;
        hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
        if (hSnap == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        MODULEENTRY32 me;
        BOOL keepGoing = !!Module32First(hSnap, &me);
        int cnt = 0;
        while (keepGoing)
        {
            LoadModule(hProcess, me.szExePath, me.szModule, (DWORD64)me.modBaseAddr,
                me.modBaseSize);
            cnt++;
            keepGoing = !!Module32Next(hSnap, &me);
        }
        CloseHandle(hSnap);
        if (cnt <= 0)
            return FALSE;
        return TRUE;
    }

    BOOL GetModuleListPSAPI(HANDLE hProcess)
    {
        DWORD i;
        //ModuleEntry  e;
        DWORD        cbNeeded;
        MODULEINFO   mi;
        HMODULE* hMods = NULL;
        PTCHAR tt = NULL;
        PTCHAR tt2 = NULL;
        const SIZE_T TTBUFLEN = 8096;
        int          cnt = 0;

        hMods = (HMODULE*)malloc(sizeof(HMODULE) * (TTBUFLEN / sizeof(HMODULE)));
        tt = (PTCHAR)malloc(sizeof(TCHAR) * TTBUFLEN);
        tt2 = (PTCHAR)malloc(sizeof(TCHAR) * TTBUFLEN);
        if ((hMods == NULL) || (tt == NULL) || (tt2 == NULL))
            goto cleanup;

        if (!EnumProcessModules(hProcess, hMods, TTBUFLEN, &cbNeeded)) {
            BM_DEBUG_ERROR_LOG("EPM failed, GetLastError: {:X}", GetLastError());
            goto cleanup;
        }

        if (cbNeeded > TTBUFLEN) {
            BM_DEBUG_ERROR_LOG("More than {:d} module handles.", TTBUFLEN);
            goto cleanup;
        }

        for (i = 0; i < cbNeeded / sizeof(hMods[0]); i++) {
            // base address, size
            GetModuleInformation(hProcess, hMods[i], &mi, sizeof(mi));
            // image file name
            tt[0] = 0;
            GetModuleFileNameEx(hProcess, hMods[i], tt, TTBUFLEN);
            // module name
            tt2[0] = 0;
            GetModuleBaseName(hProcess, hMods[i], tt2, TTBUFLEN);

            DWORD dwRes = LoadModule(hProcess, tt, tt2, (DWORD64)mi.lpBaseOfDll, mi.SizeOfImage);
            if (dwRes != ERROR_SUCCESS)
                BM_DEBUG_ERROR_LOG("StackWalk64, GetLastError: {:X} (Address: {:p})", dwRes, (LPVOID)0);
            cnt++;
        }

    cleanup:
        if (tt2 != NULL)
            free(tt2);
        if (tt != NULL)
            free(tt);
        if (hMods != NULL)
            free(hMods);

        return cnt != 0;
    }

    DWORD LoadModule(HANDLE hProcess, LPCTSTR img, LPCTSTR mod, DWORD64 baseAddr, DWORD size)
    {
        PTCHAR szImg = _strdup(img);
        PTCHAR szMod = _strdup(mod);
        DWORD result = ERROR_SUCCESS;
        if ((szImg == NULL) || (szMod == NULL))
            result = ERROR_NOT_ENOUGH_MEMORY;
        else {
            if (SymLoadModuleEx(hProcess, 0, szImg, szMod, baseAddr, size, NULL, NULL) == 0)
                result = GetLastError();
        }
        ULONGLONG fileVersion = 0;
        if (szImg != NULL) {
            // try to retrieve the file-version:
            if ((m_options & RetrieveFileVersion) != 0) {
                VS_FIXEDFILEINFO* fInfo = NULL;
                DWORD             dwHandle;
                DWORD             dwSize = GetFileVersionInfoSize(szImg, &dwHandle);
                if (dwSize > 0) {
                    LPVOID vData = malloc(dwSize);
                    if (vData != NULL) {
                        if (GetFileVersionInfo(szImg, dwHandle, dwSize, vData) != 0) {
                            UINT  len;
                            TCHAR szSubBlock[] = _T("\\");
                            if (VerQueryValue(vData, szSubBlock, (LPVOID*)&fInfo, &len) == 0)
                                fInfo = NULL;
                            else {
                                fileVersion =
                                    ((ULONGLONG)fInfo->dwFileVersionLS) + ((ULONGLONG)fInfo->dwFileVersionMS << 32);
                            }
                        }
                        free(vData);
                    }
                }
            }

            // Retrieve some additional-infos about the module
            IMAGEHLP_MODULE64 Module;
            const TCHAR* szSymType = TEXT("-unknown-");
            if (GetModuleInfo(hProcess, baseAddr, &Module) != FALSE) {
                switch (Module.SymType) {
                    case SymNone:
                        szSymType = TEXT("-nosymbols-");
                        break;
                    case SymCoff: // 1
                        szSymType = TEXT("COFF");
                        break;
                    case SymCv: // 2
                        szSymType = TEXT("CV");
                        break;
                    case SymPdb: // 3
                        szSymType = TEXT("PDB");
                        break;
                    case SymExport: // 4
                        szSymType = TEXT("-exported-");
                        break;
                    case SymDeferred: // 5
                        szSymType = TEXT("-deferred-");
                        break;
                    case SymSym: // 6
                        szSymType = TEXT("SYM");
                        break;
                    case 7: // SymDia:
                        szSymType = TEXT("DIA");
                        break;
                    case 8: //SymVirtual:
                        szSymType = TEXT("Virtual");
                        break;
                }
            }
            LPCTSTR pdbName = Module.LoadedImageName;
            if (Module.LoadedPdbName[0] != 0)
                pdbName = Module.LoadedPdbName;
            OnLoadModule(img, mod, baseAddr, size, result, szSymType, pdbName,
                fileVersion);
        }
        if (szImg != NULL)
            free(szImg);
        if (szMod != NULL)
            free(szMod);
        return result;
    }

    BOOL LoadModules(HANDLE hProcess, DWORD dwProcessId)
    {
        // first try toolhelp32
        if (GetModuleListTH32(hProcess, dwProcessId))
            return true;
        // then try psapi
        return GetModuleListPSAPI(hProcess);
    }

    static BOOL GetModuleInfo(HANDLE hProcess, DWORD64 baseAddr, IMAGEHLP_MODULE64* pModuleInfo)
    {
        memset(pModuleInfo, 0, sizeof(IMAGEHLP_MODULE64));
        // First try to use the larger ModuleInfo-Structure
        pModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        // reserve enough memory, so the bug in v6.3.5.1 does not lead to memory-overwrites...
        void* pData = malloc(4096);
        if (pData == NULL) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        memcpy(pData, pModuleInfo, sizeof(IMAGEHLP_MODULE64));
        if (SymGetModuleInfo64(hProcess, baseAddr, (IMAGEHLP_MODULE64*)pData) != FALSE) {
            // only copy as much memory as is reserved...
            memcpy(pModuleInfo, pData, sizeof(IMAGEHLP_MODULE64));
            pModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
            free(pData);
            return TRUE;
        }

        free(pData);
        SetLastError(ERROR_DLL_INIT_FAILED);
        return FALSE;
    }
};


class StackWalkerBM final : public StackWalker
{
public:
    std::basic_string<TCHAR> DumpCallStack(EXCEPTION_POINTERS* pExceptionPointers = nullptr)
    {
        // Secret windows hacks.
        static_assert(sizeof std::exception_ptr == sizeof std::shared_ptr<const _EXCEPTION_RECORD>
            && alignof(std::exception_ptr) == alignof(std::shared_ptr<const _EXCEPTION_RECORD>));

        EXCEPTION_POINTERS exceptionPointers { nullptr, nullptr };
        if (pExceptionPointers == nullptr) {
            std::exception_ptr exceptionPtr = std::current_exception();
            if (!exceptionPtr) {
                return TEXT("No callStack available");
            }
            auto exceptionRecord = static_cast<const std::shared_ptr<_EXCEPTION_RECORD>*>(reinterpret_cast<void*>(&exceptionPtr));
            exceptionPointers.ExceptionRecord = exceptionRecord->get();
        }
        else {
            exceptionPointers.ContextRecord = pExceptionPointers->ContextRecord;
            exceptionPointers.ExceptionRecord = pExceptionPointers->ExceptionRecord;
        }

        ShowCallStack(GetCurrentThread(), exceptionPointers.ContextRecord);
        const BOOL ret = WriteFullDump(GetMiniDumpFile().c_str(), &exceptionPointers);
        BM_DEBUG_TRACE_LOG("WriteFullDump -> {}", ret == TRUE);

        return callStack;
    }

    std::basic_string<TCHAR> GetCallStack()
    {
        return callStack;
    }

private:
    std::basic_string<TCHAR> GetFileVersion()
    {
        const std::basic_string<TCHAR> defaultFileVersion = TEXT("0.0.0.0");

        TCHAR dllPath[MAX_PATH];
        if (GetModuleFileName((HINSTANCE)&__ImageBase, dllPath, _countof(dllPath)) == NULL) {
            BM_DEBUG_ERROR_LOG("GetModuleFileName, GetLastError: {:X}", GetLastError());
            return defaultFileVersion;
        }

        DWORD fileVersionSize = GetFileVersionInfoSize(dllPath, NULL);
        if (fileVersionSize == NULL) {
            BM_DEBUG_ERROR_LOG("GetFileVersionInfoSize, GetLastError: {:X}", GetLastError());
            return defaultFileVersion;
        }

        std::vector<BYTE> fileVersion(fileVersionSize);
        if (GetFileVersionInfo(dllPath, NULL, fileVersionSize, fileVersion.data()) == FALSE) {
            BM_DEBUG_ERROR_LOG("GetFileVersionInfo, GetLastError: {:X}", GetLastError());
            return defaultFileVersion;
        }

        VS_FIXEDFILEINFO* pFileInfo = NULL;
        UINT pLenFileInfo = 0;
        if (!VerQueryValue(fileVersion.data(), TEXT("\\"), (LPVOID*)&pFileInfo, &pLenFileInfo)) {
            BM_DEBUG_ERROR_LOG("VerQueryValue, GetLastError: {:X}", GetLastError());
            return defaultFileVersion;
        }

        return fmt::format(TEXT("{:d}.{:d}.{:d}.{:d}"),
            (pFileInfo->dwFileVersionMS >> 16) & 0xffff,
            (pFileInfo->dwFileVersionMS) & 0xffff,
            (pFileInfo->dwFileVersionLS >> 16) & 0xffff,
            (pFileInfo->dwFileVersionLS) & 0xffff);
    }

    std::basic_string<TCHAR> GetMiniDumpFile()
    {
        TCHAR pluginName[512] = TEXT("Unknown");
        SetLastError(ERROR_SUCCESS);
        GetModuleFileName((HINSTANCE)&__ImageBase, pluginName, sizeof pluginName);
        if (GetLastError()) {
            BM_DEBUG_WARNING_LOG("GetModuleFileName, GetLastError: {:X}", GetLastError());
            strcpy_s(pluginName, TEXT("Unknown"));
        }
        else {
            PTCHAR filename = strrchr(pluginName, TEXT('\\')) + 1;
            strncpy_s(pluginName, filename, strrchr(pluginName, TEXT('.')) - filename);
        }
        

        std::basic_string<TCHAR> pluginVersion = GetFileVersion();

        std::basic_string<TCHAR> localTime = TEXT("0.0.0-0.0.0");
        tm currentTimeInfo;
        const std::time_t currentTime = std::time(nullptr);
        if (!localtime_s(&currentTimeInfo, &currentTime)) {
            localTime = fmt::format(TEXT("{:%Y.%m.%d-%H.%M.%S}"), currentTimeInfo);
        }

        std::basic_string<TCHAR> fileName = fmt::format(TEXT("{:s}-v{:s}-{:s}.dmp"), pluginName, pluginVersion, localTime);

#ifdef UNICODE
        return (BakkesModCrashesFolder / fileName).wstring();
#else
        return (BakkesModCrashesFolder / fileName).string();
#endif
    }
    
    BOOL WriteFullDump(LPCTSTR lpFileName, EXCEPTION_POINTERS* exceptionPointers)
    {
        if (exceptionPointers == nullptr) {
            return FALSE;
        }

        BM_DEBUG_TRACE_LOG(to_string(lpFileName));
        HANDLE hFile = CreateFile(lpFileName, GENERIC_ALL, NULL, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            BM_DEBUG_ERROR_LOG("CreateFile, GetLastError: {:X}", GetLastError());
            return FALSE;
        }

        HANDLE hProcess = GetCurrentProcess();
        DWORD processId = GetCurrentProcessId();
        const DWORD dumpTypeFull = MiniDumpWithFullMemory |
            MiniDumpWithFullMemoryInfo |
            MiniDumpWithHandleData |
            MiniDumpWithUnloadedModules |
            MiniDumpWithThreadInfo;
        const DWORD dumpTypeReferencedMemory = MiniDumpWithIndirectlyReferencedMemory;
        const DWORD dumpTypeNormal = MiniDumpNormal;
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = exceptionPointers;
        exceptionInfo.ClientPointers = FALSE;
        BOOL Result = MiniDumpWriteDump(hProcess,
            processId,
            hFile,
            (MINIDUMP_TYPE)dumpTypeReferencedMemory,
            &exceptionInfo,
            nullptr,
            nullptr);

        CloseHandle(hFile);

        if (!Result) {
            BM_DEBUG_ERROR_LOG("MiniDumpWriteDump, GetLastError: {:X}", GetLastError());
            return FALSE;
        }

        return TRUE;
    }
    
    void OnCallStackEntry(const CallStackEntryType eType, CallStackEntry& entry) override
    {
        if (eType != lastEntry && entry.offset != 0) {
            std::basic_string<TCHAR> function = fmt::format(TEXT("0x{:X}"), entry.offset);
            if (entry.undFullName[0] != TEXT('\0'))
                function = fmt::format(TEXT("{:s} {:s}()"), function, entry.undFullName);
            else if (entry.undName[0] != TEXT('\0'))
                function = fmt::format(TEXT("{:s} {:s}()"), function, entry.undName);
            else if (entry.name[0] != TEXT('\0'))
                function = fmt::format(TEXT("{:s} {:s}()"), function, entry.name);
            std::basic_string<TCHAR> filename = TEXT("(filename not available)");
            if (entry.lineFileName[0] != TEXT('\0'))
                filename = fmt::format(TEXT("[File={:s}:{:d}]"), entry.lineFileName, entry.lineNumber);
            std::basic_string<TCHAR> moduleName = TEXT("(module-name not available)");
            if (entry.moduleName[0] != TEXT('\0'))
                //moduleName = fmt::format("[in {:s}]", entry.moduleName);
                moduleName = fmt::format(TEXT("[in {:s}]"), entry.loadedImageName);

            callStack += fmt::format(TEXT("{:s} {:s} {:s}\n"), function, filename, moduleName);
        }
    }

    void OnOutput(LPCTSTR) override {}

    std::basic_string<TCHAR> callStack;
};

#undef strcpy_s
#undef strncpy_s
#undef _strdup
#undef strcat_s
#undef strlen
#undef strrchr

#undef IMAGEHLP_SYMBOL64
#undef PIMAGEHLP_SYMBOL64
#undef SymGetSymFromAddr64
