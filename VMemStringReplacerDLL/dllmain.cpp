#include <vector>
#include <windows.h>
#include "dllmain.h"

VOID FindAllOccurrences(PCSTR pszTarget, PCHAR pcaBuff, INT iBuffSize, std::vector<INT> &ivOccurrences);
VOID GetLpsArray(PCSTR pszPattern, INT iPatternSize, PINT piaLps);

INT DLLEXPORT WINAPI DumpMem(DWORD dwPid, PCSTR pszDumpFilePath)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
    if (!hProc) {
        return 1;
    }

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    DWORD_PTR pMaxAddr = (DWORD_PTR)sysinfo.lpMaximumApplicationAddress;
    DWORD_PTR pMinAddr = (DWORD_PTR)sysinfo.lpMinimumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;
    PCHAR pcaBuff = new CHAR[sysinfo.dwPageSize];

    FILE *pDumpFile;
    fopen_s(&pDumpFile, pszDumpFilePath, "wb");
    for (DWORD_PTR pAddr = pMinAddr; pAddr < pMaxAddr; pAddr += sysinfo.dwPageSize) {
        VirtualQueryEx(hProc, (LPCVOID)pAddr, &mbi, sizeof(mbi));
        if (mbi.Protect == PAGE_READWRITE && mbi.State == MEM_COMMIT) {
            ZeroMemory(pcaBuff, sysinfo.dwPageSize);
            ReadProcessMemory(hProc, (LPCVOID)pAddr, pcaBuff, sysinfo.dwPageSize, NULL);
            fwrite(pcaBuff, sizeof(char), sysinfo.dwPageSize, pDumpFile);
        }
    }
    fclose(pDumpFile);

    CloseHandle(hProc);
    delete[] pcaBuff;
    return 0;
}

INT DLLEXPORT WINAPI ReplaceString(DWORD dwPid, PCSTR pszTarget, PCSTR pszReplacement)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
    if (!hProc) {
        return 1;
    }

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    DWORD_PTR pMaxAddr = (DWORD_PTR)sysinfo.lpMaximumApplicationAddress;
    DWORD_PTR pMinAddr = (DWORD_PTR)sysinfo.lpMinimumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;
    PCHAR pcaBuff = new CHAR[sysinfo.dwPageSize];

    std::vector<INT> ivOccurrences;
    for (DWORD_PTR pAddr = pMinAddr; pAddr < pMaxAddr; pAddr += sysinfo.dwPageSize) {
        VirtualQueryEx(hProc, (LPCVOID)pAddr, &mbi, sizeof(mbi));
        if (mbi.Protect == PAGE_READWRITE && mbi.State == MEM_COMMIT) {
            ZeroMemory(pcaBuff, sysinfo.dwPageSize);
            ReadProcessMemory(hProc, (LPCVOID)pAddr, pcaBuff, sysinfo.dwPageSize, NULL);
            FindAllOccurrences(pszTarget, pcaBuff, sysinfo.dwPageSize, ivOccurrences);
            if (ivOccurrences.size()) {
                for (INT &iOccurrence : ivOccurrences) {
                    for (INT i = 0; pszTarget[i]; i++) {
                        pcaBuff[iOccurrence + i] = pszReplacement[i];
                    }
                }
                WriteProcessMemory(hProc, (LPVOID)pAddr, pcaBuff, sysinfo.dwPageSize, NULL);
                ivOccurrences.clear();
            }
        }
    }

    CloseHandle(hProc);
    delete[] pcaBuff;
    return 0;
}

VOID FindAllOccurrences(PCSTR pszTarget, PCHAR pcaBuff, INT iBuffSize, std::vector<INT> &ivOccurrences)
{
    INT iTargetLength = strlen(pszTarget);
    PINT piaLps = new INT[iTargetLength];
    GetLpsArray(pszTarget, iTargetLength, piaLps);

    INT iBuffIndex = 0;
    INT iTargetIndex = 0;
    while (iBuffIndex < iBuffSize) {
        if (pszTarget[iTargetIndex] == pcaBuff[iBuffIndex]) {
            iBuffIndex++;
            iTargetIndex++;
        }

        if (iTargetIndex == iTargetLength) {
            ivOccurrences.push_back(iBuffIndex - iTargetIndex);
            iTargetIndex = 0;
        } else if (iBuffIndex < iBuffSize && pszTarget[iTargetIndex] != pcaBuff[iBuffIndex]) {
            if (iTargetIndex) {
                iTargetIndex = piaLps[iTargetIndex - 1];
            } else {
                iBuffIndex++;
            }
        }
    }

    delete[] piaLps;
}

VOID GetLpsArray(PCSTR pszPattern, INT iPatternSize, PINT piaLps)
{
    INT iLpsLength = 0;
    piaLps[0] = 0;

    INT i = 1;
    while (i < iPatternSize) {
        if (pszPattern[i] == pszPattern[iLpsLength]) {
            piaLps[i++] = ++iLpsLength;
        } else if (iLpsLength) {
            iLpsLength = piaLps[iLpsLength - 1];
        } else {
            piaLps[i++] = 0;
        }
    }
}
