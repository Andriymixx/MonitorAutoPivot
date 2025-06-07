
#include "gui.h"
#include <Windows.h>
#include <thread>
#include <string>
#pragma comment(lib, "gdi32.lib")
const wchar_t* WINDOW_DATA_KEY = L"MonitorInfoKey";

// show window on a specified monitor
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE: {
		// Get info about monitor
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		MonitorInfo* monitorInfo = reinterpret_cast<MonitorInfo*>(pCreate->lpCreateParams);

		// Set data in a corresponding window
		SetProp(hwnd, WINDOW_DATA_KEY, monitorInfo);
		// Timer for destroy window
		SetTimer(hwnd, 1, 5000, NULL);
		return 0;
	}
	case WM_TIMER:
		KillTimer(hwnd, 1);
		DestroyWindow(hwnd);
		return 0;
	case WM_PAINT: {
		MonitorInfo* monitorInfo = reinterpret_cast<MonitorInfo*>(GetProp(hwnd, WINDOW_DATA_KEY));
		if (!monitorInfo) return 0;
		// Drawing window
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		RECT rect;
		GetClientRect(hwnd, &rect);
		HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		HFONT hFont = CreateFont(50, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
			DEFAULT_PITCH | FF_SWISS, L"Arial");
		HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);
		// Specifying monitor index to display
		std::wstring message = L"Monitor #" + std::to_wstring(monitorInfo->index);
		DrawText(hdc, message.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		SelectObject(hdc, hOldFont);
		DeleteObject(hFont);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
			return 0;
		}
		break;
	case WM_DESTROY: {
		MonitorInfo* monitorInfo = reinterpret_cast<MonitorInfo*>(GetProp(hwnd, WINDOW_DATA_KEY));
		if (monitorInfo) {
			RemoveProp(hwnd, WINDOW_DATA_KEY);
			delete monitorInfo;
		}
		return 0;
	}
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int RunWindowMessageLoopAsync(std::vector<MonitorInfo> monitors) {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	const wchar_t* className = L"MonitorMessageClass";

	// Register the window class only if it is not already registered
	WNDCLASSEX wcTest = {};
	if (!GetClassInfoExW(hInstance, className, &wcTest)) {
		WNDCLASSEX wc = {};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = hInstance;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpszClassName = className;
		if (!RegisterClassEx(&wc)) {
			MessageBox(NULL, L"Could not register window class.", L"Error", MB_OK | MB_ICONERROR);
			return 1;
		}
	}

	std::vector<HWND> windows;
	// For each monitor
	for (const auto& monitor : monitors) {
		MonitorInfo* monitorInfoCopy = new MonitorInfo(monitor);
		// Get monitor`s position
		int posX = 0;
		int posY = 0;
		DEVMODE devMode = {};
		devMode.dmSize = sizeof(DEVMODE);
		if (EnumDisplaySettings(monitor.displayNum.c_str(), ENUM_CURRENT_SETTINGS, &devMode)) {
			// set some padding to displayed window from monitor`s corner
			posX = devMode.dmPosition.x + 50;
			posY = devMode.dmPosition.y + 50;
		}
		int winWidth = 400, winHeight = 200;
		// Set monitor index to window title
		std::wstring windowTitle = L"Monitor #" + std::to_wstring(monitor.index);
		HWND hwnd = CreateWindowEx(
			WS_EX_TOPMOST, L"MonitorMessageClass", windowTitle.c_str(),
			WS_POPUP, posX, posY, winWidth, winHeight,
			NULL, NULL, hInstance, monitorInfoCopy
		);
		if (hwnd) {
			windows.push_back(hwnd);
			ShowWindow(hwnd, SW_SHOW);
			UpdateWindow(hwnd);
		}
		else {
			delete monitorInfoCopy;
		}
	}
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}