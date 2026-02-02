# 🚀 Lightning Speed GitHub Desktop for WSL

## 🔴 The Problem
GitHub Desktop is notoriously slow when working with repositories inside WSL (Windows Subsystem for Linux). This happens because it uses a Windows-native Git binary to read Linux files over a slow network bridge (9P protocol). Large repositories can take seconds just to refresh or show a diff.

## 🟢 The Solution: WSL Git Bridge
This bridge replaces the internal Windows Git binary with a high-speed proxy. Instead of Windows trying to read Linux files, this bridge tells WSL to run the Git command **natively inside Linux** and sends the result back to the Windows app instantly.

---

## 🛠️ Installation

1.  **Close GitHub Desktop.**
2.  **Locate the Git folder** inside your GitHub Desktop installation. It is usually found here:
    `%LOCALAPPDATA%\GitHubDesktop\app-X.X.X\resources\app\git\cmd`
    *(Replace X.X.X with your current version number, e.g., 3.5.5-beta2)*.
3.  **Backup the original:** Rename the existing `git.exe` in that folder to `git-original.exe`.
4.  **Install the Bridge:** Copy this compiled `git.exe` (the bridge) into that same folder.
5.  **Restart GitHub Desktop.**

---

## 💎 Why it's better
*   **Instant Refresh:** Operations like "fetch", "status", and "diff" load in milliseconds.
*   **Pure Data:** Bypasses the Windows shell (`cmd.exe`) to prevent "Identity" and "Path" corruption errors.
*   **Native WSL:** Executes Git directly inside your Ubuntu/Debian environment with full filesystem speed.

---

## 📜 Source Code (C)
For security and transparency, here is the source code for the bridge. It can be compiled using `x86_64-w64-mingw32-gcc`.

```c
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

/* 
   Sharp & Efficient WSL Git Bridge v29 by MD ⚡
   - Bypasses cmd.exe/shell entirely using CreateProcess.
   - Zero shell pollution or injected warnings.
   - 100% Binary Safe I/O.
*/

int main(int argc, char *argv[]) {
    _setmode(_fileno(stdout), _O_BINARY);
    
    char win_cwd[4096];
    char wsl_cwd[4096] = "";
    if (GetCurrentDirectory(sizeof(win_cwd), win_cwd) != 0) {
        char *wsl_ptr = strstr(win_cwd, "\\\\wsl.localhost\\Ubuntu");
        if (!wsl_ptr) wsl_ptr = strstr(win_cwd, "\\\\wsl$\\Ubuntu");
        
        if (wsl_ptr) {
            strcpy(wsl_cwd, wsl_ptr + (strstr(win_cwd, "wsl.localhost") ? 22 : 13));
            for (int i = 0; wsl_cwd[i]; i++) if (wsl_cwd[i] == '\\') wsl_cwd[i] = '/';
        }
    }

    char cmdline[32768] = "wsl.exe ";
    if (wsl_cwd[0]) {
        strcat(cmdline, "--cd \"");
        strcat(cmdline, wsl_cwd);
        strcat(cmdline, "\" ");
    }
    strcat(cmdline, "-e git");
    for (int i = 1; i < argc; i++) {
        strcat(cmdline, " \"");
        strcat(cmdline, argv[i]);
        strcat(cmdline, "\"");
    }

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) return 1;
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) return 1;

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = NULL; 
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &piProcInfo)) return 1;

    CloseHandle(hChildStd_OUT_Wr);

    DWORD dwRead;
    unsigned char chBuf[16384];
    int first_block = 1;
    while (ReadFile(hChildStd_OUT_Rd, chBuf, sizeof(chBuf), &dwRead, NULL) && dwRead > 0) {
        DWORD offset = 0;
        if (first_block) {
            first_block = 0;
            if (dwRead > 10 && (chBuf[0] == 'C' && chBuf[1] == ':' && chBuf[2] == '\\')) {
                while (offset < dwRead && chBuf[offset] != '\n') offset++;
                if (offset < dwRead) offset++;
            }
        }
        if (dwRead > offset) {
            fwrite(chBuf + offset, 1, dwRead - offset, stdout);
        }
    }

    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(piProcInfo.hProcess, &exit_code);
    
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    CloseHandle(hChildStd_OUT_Rd);

    return (int)exit_code;
}
```
