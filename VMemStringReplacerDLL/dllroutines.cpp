#include <vector>
#include <windows.h>
#include "dllroutines.h"

void FindAllOccurrences(const char *pszTarget, const char *pcaBuff, int iBuffSize, std::vector<int> &occurrences);
void GetLpsArray(const char *pszPattern, int iPatternSize, int *piaLps);

void DLLEXPORT __stdcall DumpMem(const char *pszDumpFilePath)
{
    HANDLE hCurrProc = GetCurrentProcess();
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    DWORD_PTR pMaxAddr = (DWORD_PTR)sysinfo.lpMaximumApplicationAddress;
    DWORD_PTR pMinAddr = (DWORD_PTR)sysinfo.lpMinimumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;
    char *pcaBuff = new char[sysinfo.dwPageSize];

    FILE *pDumpFile;
    fopen_s(&pDumpFile, pszDumpFilePath, "wb");
    for (DWORD_PTR pAddr = pMinAddr; pAddr < pMaxAddr; pAddr += sysinfo.dwPageSize) {
        VirtualQueryEx(hCurrProc, (LPCVOID)pAddr, &mbi, sizeof(mbi));
        if (mbi.Protect == PAGE_READWRITE && mbi.State == MEM_COMMIT) {
            ZeroMemory(pcaBuff, sysinfo.dwPageSize);
            ReadProcessMemory(hCurrProc, (LPCVOID)pAddr, pcaBuff, sysinfo.dwPageSize, NULL);
            fwrite(pcaBuff, sizeof(char), sysinfo.dwPageSize, pDumpFile);
        }
    }
    fclose(pDumpFile);

    delete[] pcaBuff;
}

void DLLEXPORT __stdcall ReplaceString(const char *pszTarget, const char *pszReplacement)
{
    HANDLE hCurrProc = GetCurrentProcess();
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    DWORD_PTR pMaxAddr = (DWORD_PTR)sysinfo.lpMaximumApplicationAddress;
    DWORD_PTR pMinAddr = (DWORD_PTR)sysinfo.lpMinimumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;
    char *pcaBuff = new char[sysinfo.dwPageSize];

    std::vector<int> occurrences;
    for (DWORD_PTR pAddr = pMinAddr; pAddr < pMaxAddr; pAddr += sysinfo.dwPageSize) {
        VirtualQueryEx(hCurrProc, (LPCVOID)pAddr, &mbi, sizeof(mbi));
        if (mbi.Protect == PAGE_READWRITE && mbi.State == MEM_COMMIT) {
            ZeroMemory(pcaBuff, sysinfo.dwPageSize);
            ReadProcessMemory(hCurrProc, (LPCVOID)pAddr, pcaBuff, sysinfo.dwPageSize, NULL);
            FindAllOccurrences(pszTarget, pcaBuff, sysinfo.dwPageSize, occurrences);
            if (occurrences.size()) {
                for (int &iOccurrence : occurrences) {
                    for (int i = 0; pszTarget[i]; i++) {
                        pcaBuff[iOccurrence + i] = pszTarget[i];
                    }
                }
                WriteProcessMemory(hCurrProc, (LPVOID)pAddr, pcaBuff, sysinfo.dwPageSize, NULL);
                occurrences.clear();
            }
        }
    }

    delete[] pcaBuff;
}

void FindAllOccurrences(const char *pszTarget, const char *pcaBuff, int iBuffSize, std::vector<int> &occurrences)
{
    int iTargetLength = strlen(pszTarget);
    int *piaLps = new int[iTargetLength];
    GetLpsArray(pszTarget, iTargetLength, piaLps);

    int iBuffIndex = 0;
    int iTargetIndex = 0;
    while (iBuffIndex < iBuffSize) {
        if (pszTarget[iTargetIndex] == pcaBuff[iBuffIndex]) {
            iBuffIndex++;
            iTargetIndex++;
        }

        if (iTargetIndex == iTargetLength) {
            occurrences.push_back(iBuffIndex - iTargetIndex);
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

void GetLpsArray(const char *pszPattern, int iPatternSize, int *piaLps)
{
    int iLpsLength = 0;
    piaLps[0] = 0;

    int i = 1;
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
