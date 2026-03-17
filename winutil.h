/**
 * winutil.h
 * Simplified Windows API functions
 *
 * Copyright (c) 2026 HGS Team
 * Author: Administrator
 *
 * Usage examples:
 *   msgbox("Hello World");                    // Message box
 *   find("Notepad");                          // Find notepad window
 *   setText(hwnd, "New Title");                // Set window text
 *   show(hwnd, 1);                             // Show window
 *   lastError();                                // Get last error
 *   beep();                                     // Default beep
 *   beep(1000, 500);                            // Beep 1000Hz for 500ms
 *   getX(); getY();                              // Get cursor position
 *   exec("notepad.exe");                         // Run program
 *   hideConsole();                                // Hide console window
 * 
 * Note: When using with cout, remember to add flush if needed.
 * For example: cout << "Message" << flush;
 */

#ifndef WINUTIL_H
#define WINUTIL_H

#include <windows.h>
#include <string>

// -------------------- Window Find & Management --------------------

inline HWND find(const std::string& title) {
    // Find window by title, return window handle
    return FindWindowA(NULL, title.c_str());
}

inline HWND findClass(const std::string& className) {
    // Find window by class name
    return FindWindowA(className.c_str(), NULL);
}

inline void setText(HWND hwnd, const std::string& text) {
    // Set window title
    SetWindowTextA(hwnd, text.c_str());
}

inline std::string getText(HWND hwnd) {
    // Get window title
    char buffer[256];
    GetWindowTextA(hwnd, buffer, 256);
    return std::string(buffer);
}

inline void show(HWND hwnd, int cmd = 1) {
    // Show/hide window: 0=hide, 1=normal, 2=minimize, 3=maximize
    ShowWindow(hwnd, cmd);
}

inline void update(HWND hwnd) {
    // Refresh window
    UpdateWindow(hwnd);
}

inline void close(HWND hwnd) {
    // Send close message
    PostMessage(hwnd, WM_CLOSE, 0, 0);
}

// -------------------- Dialog & Message --------------------

inline void msgbox(const std::string& text) {
    // Simple message box
    MessageBoxA(NULL, text.c_str(), "Message", MB_OK);
}

inline void msgbox(const std::string& text, const std::string& title) {
    // Message box with custom title
    MessageBoxA(NULL, text.c_str(), title.c_str(), MB_OK);
}

inline int ask(const std::string& question) {
    // Yes/No question box, returns 6=Yes, 7=No
    return MessageBoxA(NULL, question.c_str(), "Question", MB_YESNO);
}

inline void warn(const std::string& text) {
    // Warning box
    MessageBoxA(NULL, text.c_str(), "Warning", MB_OK | MB_ICONWARNING);
}

inline void error(const std::string& text) {
    // Error box
    MessageBoxA(NULL, text.c_str(), "Error", MB_OK | MB_ICONERROR);
}

// -------------------- Error Handling --------------------

inline DWORD lastError() {
    // Get last error code
    return GetLastError();
}

inline std::string errorStr(DWORD code = 0) {
    // Convert error code to text description
    if (code == 0) code = GetLastError();
    
    char* msgBuffer = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, code, 0, (LPSTR)&msgBuffer, 0, NULL
    );
    
    std::string result = msgBuffer ? msgBuffer : "Unknown error";
    LocalFree(msgBuffer);
    return result;
}

// -------------------- File Operations --------------------

inline HANDLE openFile(const std::string& filename, bool readOnly = true) {
    // Open file, return handle
    DWORD access = readOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
    DWORD share = readOnly ? FILE_SHARE_READ : 0;
    
    return CreateFileA(
        filename.c_str(),
        access,
        share,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
}

inline HANDLE createFile(const std::string& filename) {
    // Create new file
    return CreateFileA(
        filename.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
}

inline bool readFile(HANDLE file, void* buffer, DWORD bytesToRead) {
    // Read from file
    DWORD bytesRead;
    return ReadFile(file, buffer, bytesToRead, &bytesRead, NULL);
}

inline bool writeFile(HANDLE file, const void* buffer, DWORD bytesToWrite) {
    // Write to file
    DWORD bytesWritten;
    return WriteFile(file, buffer, bytesToWrite, &bytesWritten, NULL);
}

inline void closeHandle(HANDLE handle) {
    // Close handle
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
}

// -------------------- System & Hardware --------------------

inline void beep() {
    // Default beep
    MessageBeep(MB_OK);
}

inline void beep(DWORD frequency, DWORD duration) {
    // Beep with specific frequency (Hz) and duration (ms)
    Beep(frequency, duration);
}

inline int getX() {
    // Get cursor X position
    POINT p;
    GetCursorPos(&p);
    return p.x;
}

inline int getY() {
    // Get cursor Y position
    POINT p;
    GetCursorPos(&p);
    return p.y;
}

inline void gotoSleep(int ms) {
    // Sleep for milliseconds
    Sleep(ms);
}

// -------------------- Process & Console --------------------

inline bool exec(const std::string& command) {
    // Execute a program
    int result = (int)ShellExecuteA(
        NULL,
        "open",
        command.c_str(),
        NULL,
        NULL,
        SW_SHOWNORMAL
    );
    return result > 32;  // ShellExecute returns >32 on success
}

inline void hideConsole() {
    // Hide console window
    ShowWindow(GetConsoleWindow(), SW_HIDE);
}

inline void showConsole() {
    // Show console window
    ShowWindow(GetConsoleWindow(), SW_SHOW);
}

inline HWND getConsoleWindow() {
    // Get console window handle
    return GetConsoleWindow();
}

// -------------------- Timer & Performance --------------------

inline DWORD tickCount() {
    // Get system uptime in milliseconds
    return GetTickCount();
}

inline LARGE_INTEGER perfCounter() {
    // Get high-resolution performance counter
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter;
}

inline LARGE_INTEGER perfFrequency() {
    // Get high-resolution performance frequency
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency;
}

#endif
