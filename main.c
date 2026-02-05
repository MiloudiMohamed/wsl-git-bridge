#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

/* 
   WSL Git Bridge ⚡
   Bypasses cmd.exe/shell to provide a high-speed pipe to WSL Git.
*/

int main(int argc, char *argv[]) {
    _setmode(_fileno(stdout), _O_BINARY);
    
    char win_cwd[4096];
    char wsl_cwd[4096] = "";
    if (GetCurrentDirectory(sizeof(win_cwd), win_cwd) != 0) {
        char *wsl_ptr = strstr(win_cwd, "\\\\wsl.localhost\\");
        if (!wsl_ptr) wsl_ptr = strstr(win_cwd, "\\\\wsl$\\");
        
        if (wsl_ptr) {
            char *start = wsl_ptr + (strstr(win_cwd, "wsl.localhost") ? 16 : 7);
            char *slash = strchr(start, '\\');
            if (slash) {
                strcpy(wsl_cwd, slash);
                for (int i = 0; wsl_cwd[i]; i++) if (wsl_cwd[i] == '\\') wsl_cwd[i] = '/';
            }
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
