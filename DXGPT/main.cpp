#include <iostream>
#include <Windows.h>

#include "Dx11App.h"

// disable SAL anotation warning
#pragma warning(disable: 28251)

// Function prototypes
LRESULT CALLBACK WndProc(HWND&, UINT, WPARAM, LPARAM);
HRESULT InitWindow(HINSTANCE, int, HWND&);

void OpenConsoleWindow();

// Entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    OpenConsoleWindow();

    HWND hWnd = nullptr;

    if (FAILED(InitWindow(hInstance, nCmdShow, hWnd)))
        return 0;

    Dx11App app;
    if (FAILED(app.Init(hWnd))) {
        return 0;
    }

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            app.Render();
        }
    }

    //app.Cleanup();

    return (int)msg.wParam;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

// Initialize window
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow, HWND& hWnd) {
    // Register window class
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"TriangleApp";
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    hWnd = CreateWindow(
        L"TriangleApp",
        L"DirectX Example - Triangle",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd)
        return E_FAIL;

    // Show window
    ShowWindow(hWnd, nCmdShow);

    return S_OK;
}

void OpenConsoleWindow() {
    if (!AllocConsole()) {
        MessageBox(nullptr, L"Failed to allocate console", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    std::wcout.clear();

    FILE* pCin;
    freopen_s(&pCin, "CONIN$", "r", stdin);
    std::wcin.clear();

    SetConsoleTitle(L"Console");

    // Optional: Adjust console window position
    HWND consoleWnd = GetConsoleWindow();
    RECT rect;
    GetWindowRect(consoleWnd, &rect);
    MoveWindow(consoleWnd, rect.left, rect.top, 800, 600, TRUE);
}