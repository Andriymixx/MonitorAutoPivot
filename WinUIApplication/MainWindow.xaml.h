#pragma once
#include "MainWindow.g.h"
#include "pch.h"
#include <Shellapi.h>
#include <CommCtrl.h>
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_RESTORE 1002
#pragma comment(lib, "ComCtl32.lib") // потрібна для SetWindowSubclass
#pragma comment(lib, "Shell32.lib")  // для Shell_NotifyIcon
#include "MainPage.xaml.h"
namespace winrt::WinUIApplication::implementation
{
	struct MainWindow : MainWindowT<MainWindow>
	{
		MainWindow();
		~MainWindow();
	public:
		HWND GetWindowHandle();
	private:
		winrt::Microsoft::UI::Xaml::UIElement m_originalContent{ nullptr };
		HWND _hwnd{ nullptr };
		NOTIFYICONDATA _nid{};
		HMENU _trayMenu{ nullptr };
		void InitializeTray();
		void RemoveTray();
		static LRESULT CALLBACK SubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
		void SetWindowSize(HWND hwnd, const int width, const int height);
		void LoadIcon(HWND hwnd, wchar_t const* iconName);
		void ClipOrCenterRectToMonitorWin32(RECT& rc);
		void PlacementCenterWindowInMonitorWin32(HWND hwnd);
	};
}

namespace winrt::WinUIApplication::factory_implementation
{
	struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
	{
	};
}
