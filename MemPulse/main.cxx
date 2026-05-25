#include <windows.h>
#include <commctrl.h>
#include <string>

// 链接通用控件库
#pragma comment(lib, "comctl32.lib")

// 全局变量
HINSTANCE hInst;
HWND hEdit, hUpDown;
void* g_pBuffer = NULL;
size_t g_currentSizeMB = 0;

// 更新内存申请的函数
void UpdateMemoryAllocation(int megabytes) {
    // 1. 释放之前申请的内存
    if (g_pBuffer) {
        VirtualFree(g_pBuffer, 0, MEM_RELEASE);
        g_pBuffer = NULL;
    }

    g_currentSizeMB = (megabytes < 0) ? 0 : megabytes;

    // 2. 申请新内存
    if (g_currentSizeMB > 0) {
        size_t bytes = g_currentSizeMB * 1024 * 1024;
        g_pBuffer = VirtualAlloc(NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        if (g_pBuffer) {
            // 写入一点数据确保物理内存被真正占用（防止延迟分配）
            memset(g_pBuffer, 0, 1024);
        }
    }
}

// 窗口过程回调
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // 初始化通用控件
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_UPDOWN_CLASS;
        InitCommonControlsEx(&icex);

        // 创建编辑框 (Buddy Window)
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"0",
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            20, 20, 100, 25, hWnd, NULL, hInst, NULL);

        // 创建微调按钮 (Up-Down Control)
        hUpDown = CreateWindowEx(0, UPDOWN_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK,
            0, 0, 0, 0, hWnd, NULL, hInst, NULL);

        // 绑定编辑框
        SendMessage(hUpDown, UDM_SETBUDDY, (WPARAM)hEdit, 0);
        // 设置范围 0 - 2048 MB
        SendMessage(hUpDown, UDM_SETRANGE, 0, MAKELONG(2048, 0));
        break;
    }

    case WM_VSCROLL: // 当 UpDown 控件被点击时会发送此消息给父窗口
        if ((HWND)lParam == hUpDown) {
            int pos = (int)SendMessage(hUpDown, UDM_GETPOS32, 0, 0);
            UpdateMemoryAllocation(pos);

            // 在窗口标题显示当前状态
            wchar_t status[100];
            wsprintf(status, L"Memory Allocator - Allocated: %d MB", pos);
            SetWindowText(hWnd, status);
        }
        break;

    case WM_DESTROY:
        UpdateMemoryAllocation(0); // 清理内存
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 入口函数
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    const wchar_t* CLASS_NAME = L"MemAllocClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(0, CLASS_NAME, L"Memory Allocator",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 300, 150,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}