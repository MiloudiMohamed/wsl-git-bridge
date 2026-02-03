# WSL Git Bridge

A high-speed proxy that fixes the extreme lag in GitHub Desktop when working with WSL repositories.

## The Problem
GitHub Desktop for Windows is notoriously slow when handling files on the WSL filesystem. This is because it uses a Windows-native Git binary that has to communicate over the slow 9P network bridge to Linux. 

## The Solution
This tool replaces the bundled Git in GitHub Desktop with a small C bridge. When the app calls Git, the bridge forwards the command directly into WSL (`wsl.exe -e git`), executes it natively on the Linux filesystem, and returns the result. 

This results in near-instant refreshes and diffs.

## Prerequisites
Make sure you have Git and Git LFS installed **inside WSL**:
```bash
sudo apt install git git-lfs -y
git lfs install
```

Confirm they're installed:
```bash
git --version
git lfs version
```

## Installation
1. Close GitHub Desktop.
2. Navigate to your GitHub Desktop Git folder: 
   `%LOCALAPPDATA%\GitHubDesktop\app-[VERSION]\resources\app\git\cmd`
3. Rename `git.exe` to `git-original.exe` (backup).
4. Copy the compiled `git.exe` from this project into that folder.
5. Restart GitHub Desktop.

## Technical Details
The bridge bypasses `cmd.exe` and uses the native Windows API (`CreateProcess`) to pipe commands directly to WSL. This ensures no shell warnings or hidden carriage returns (`\r`) corrupt the Git data stream, which often causes the "Couldn't parse identity" error in other wrappers.

## Building from source
Use `mingw-w64`:
```bash
x86_64-w64-mingw32-gcc main.c -o git.exe
```
