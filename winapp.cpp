/*
BSD 2-Clause License

Copyright (c) 2016, Vlad Mesco
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef UNICODE
# define UNICODE
#endif
#ifndef _UNICODE
# define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <tchar.h>
#include <Shellapi.h>

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <map>
#include <utility>
#include <functional>
#include <string>
#include <sstream>

// ==========================================================
// Global State
// ==========================================================

bool g_debug = false;
TCHAR g_txt[MAX_PATH+1];
HWND hWnd = NULL;

// ==========================================================
// Utilities
// ==========================================================

std::wstring operator "" _ws(const wchar_t* s, size_t n)
{
    return std::wstring(s);
}

static void Help(LPWSTR);
static std::map<wchar_t, std::tuple<bool, std::wstring, std::function<void(LPWSTR)>>> g_commandArgs {
    {L'?', std::make_tuple(false, L"Shows this message"_ws, std::function<void(LPWSTR)>(Help))},
    {L'D', std::make_tuple(false, L"Breaks for debugger"_ws, [](LPWSTR) { g_debug = true; })}
};

static FILE* GetLogFile()
{
    GetTempFileName(_T("."), _T("jak"), 0, g_txt);

    return _tfopen(g_txt, _T("w"));
}

void Log(LPTSTR format, ...)
{
    static FILE* f = GetLogFile();

    if(!f) return;

    va_list args;
    va_start(args, format);
    _vftprintf(f, format, args);
    fflush(f);
    va_end(args);
}

static void Help(LPWSTR)
{
    std::wstringstream ss;
    for(auto&& i : g_commandArgs)
    {
        if(std::get<0>(i.second)) {
            Log(L"/%carg\t%s\n", i.first, std::get<1>(i.second).c_str());
            ss << L'/' << i.first << L"arg\t" << std::get<1>(i.second).c_str() << std::endl;
        } else {
            Log(L"/%c\t%s\n", i.first, std::get<1>(i.second).c_str());
            ss << L'/' << i.first << L"\t" << std::get<1>(i.second).c_str() << std::endl;
        }
    }
    MessageBox(hWnd, ss.str().c_str(), L"WinApp", MB_OK);
    exit(2);
}

static void ParseArgs()
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(!argv)
    {
        Log(L"Could not get command line arguments: %d\n", GetLastError());
        return;
    }

    for(size_t i = 1; i < argc; ++i)
    {
        auto&& arg = argv[i];
        if(arg[0] != L'/' || !arg[1])
        {
            Log(L"invalid argument %d: %s\n", i, arg);
            continue;
        }
        auto&& found = g_commandArgs.find(arg[1]);
        std::get<2>(found->second)(arg);
    }
}

// ==========================================================
// WinApi
// ==========================================================

LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);

int WINAPI wWinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine,
        int nCmdShow)
{
    SetProcessDPIAware();
    Log(_T("Launched with: %s\n"), lpCmdLine);

    ParseArgs();

    if(g_debug) __debugbreak();

    const wchar_t CLASS_NAME[] = _T("Form1");
    WNDCLASS wc = {};

    HCURSOR crsr = LoadCursor(NULL, IDC_ARROW);

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = crsr;

    RegisterClass(&wc);

    TCHAR windowText[256] = _T("\0");
    _tcscat(windowText, _T("Log file is in: "));
    _tcsncat(windowText, g_txt, 100);
    windowText[116] = L'\0';

    HWND hwnd = CreateWindowEx(
            0, // styles
            CLASS_NAME,
            windowText, // text
            WS_OVERLAPPEDWINDOW, // style
            CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT, // size & pos
            NULL, // parent
            NULL, // menu
            hInstance, // instance handle
            NULL); // application data

    if(hwnd == NULL)
    {
        Log(_T("Failed to create window %d\n"), GetLastError());
    }

    hWnd = hwnd;

    ShowWindow(hwnd, nCmdShow);

    // Message pump
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_DESTROY:
            FreeConsole();
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
                EndPaint(hwnd, &ps);
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
