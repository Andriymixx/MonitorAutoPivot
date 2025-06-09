#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"

#include "microsoft.ui.xaml.window.h" 
#include "SampleConfiguration.h"
#endif
#include <App.xaml.h>
#include <g_flags.h>

namespace winrt
{
	using namespace Microsoft::UI::Xaml;
}

namespace winrt::WinUIApplication::implementation
{
	MainWindow::MainWindow()
	{
		InitializeComponent();
		Title(winrt::WinUIApplication::SampleConfig::FeatureName);
		auto mainPage = winrt::WinUIApplication::MainPage();
		winrt::WinUIApplication::implementation::App::rootPage = mainPage;
		m_originalContent = mainPage.as<winrt::Microsoft::UI::Xaml::UIElement>();
		RootGrid().Children().Append(m_originalContent);

		// Set a link to this window in the MainPage
		auto mainPageImpl = winrt::get_self<winrt::WinUIApplication::implementation::MainPage>(mainPage);
		mainPageImpl->SetMainWindow(*this);
		HWND hwnd = GetWindowHandle();
		LoadIcon(hwnd, L"Assets/IconLogo.ico");
		SetWindowSize(hwnd, 1050, 800);
		
		PlacementCenterWindowInMonitorWin32(hwnd);

		InitializeTray();
		SetWindowSubclass(hwnd, SubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
	}
	MainWindow::~MainWindow()
	{
		RemoveTray();
	}

	void MainWindow::InitializeTray()
	{
		// Tray icon initialization
		HWND hwnd = GetWindowHandle();
		_trayMenu = CreatePopupMenu();
		AppendMenuW(_trayMenu, MF_STRING, ID_TRAY_RESTORE, L"Restore");
		AppendMenuW(_trayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
		_nid.cbSize = sizeof(NOTIFYICONDATA);
		_nid.hWnd = hwnd;
		_nid.uID = 1;
		_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		_nid.uCallbackMessage = WM_TRAYICON;
		_nid.hIcon = static_cast<HICON>(LoadImageW(nullptr, L"Assets/IconLogo.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE));
		wcscpy_s(_nid.szTip, L"WinUIApplication");
		Shell_NotifyIconW(NIM_ADD, &_nid);
	}

	void MainWindow::RemoveTray()
	{
		Shell_NotifyIconW(NIM_DELETE, &_nid);
		if (_trayMenu) DestroyMenu(_trayMenu);
		if (_nid.hIcon) DestroyIcon(_nid.hIcon);
	}

	LRESULT CALLBACK MainWindow::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
	{
		auto* pThis = reinterpret_cast<MainWindow*>(dwRefData);

		switch (msg)
		{
		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
			lpMMI->ptMinTrackSize.x = 800; 
			lpMMI->ptMinTrackSize.y = 600; 
			return 0; 
		}
		case WM_TRAYICON:
			if (lParam == WM_RBUTTONUP)
			{
				POINT pt;
				GetCursorPos(&pt);
				SetForegroundWindow(hwnd);
				TrackPopupMenu(pThis->_trayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
			}
			else if (lParam == WM_LBUTTONDBLCLK)
			{
				ShowWindow(hwnd, SW_RESTORE);
				SetForegroundWindow(hwnd);
			}
			return 0;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case ID_TRAY_RESTORE:
				ShowWindow(hwnd, SW_RESTORE);
				SetForegroundWindow(hwnd);
				return 0;
			case ID_TRAY_EXIT:
				StopDataReceiving();
				stopMonitorWatcher = true;
				pThis->RemoveTray();
				RemoveWindowSubclass(hwnd, SubclassProc, 1);
				DestroyWindow(hwnd);
				PostQuitMessage(0);
				return 0;
			}
			break;

		case WM_CLOSE:
		{
			// If g_minimizeOnClose set in config close button acts like minimize
			if (ConfigUtil::g_minimizeOnClose)
			{
				ShowWindow(hwnd, SW_HIDE);
				return 0;
			}
			else
			{
				StopDataReceiving();
				stopMonitorWatcher = true;
				pThis->RemoveTray();
				RemoveWindowSubclass(hwnd, SubclassProc, 1);
				DestroyWindow(hwnd);
				PostQuitMessage(0);
				return 0;
			}
		}
		case WM_SYSCOMMAND:
			if ((wParam & 0xFFF0) == SC_MINIMIZE)
			{
				ShowWindow(hwnd, SW_HIDE);
				return 0;
			}
			break;
		}

		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
	HWND MainWindow::GetWindowHandle()
	{
		if (_hwnd == nullptr)
		{
			Window window = *this;
			window.as<IWindowNative>()->get_WindowHandle(&_hwnd);
		}
		return _hwnd;
	}

	void MainWindow::LoadIcon(HWND hwnd, wchar_t const* iconPath)
	{
		HANDLE hSmallIcon = LoadImageW(nullptr, iconPath, IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
			LR_LOADFROMFILE | LR_SHARED);
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hSmallIcon));

		HANDLE hBigIcon = LoadImageW(nullptr, iconPath, IMAGE_ICON,
			GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
			LR_LOADFROMFILE | LR_SHARED);
		SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hBigIcon));
	}

	void MainWindow::SetWindowSize(HWND hwnd, int width, int height)
	{
		// Win32 uses pixels and WinUI 3 uses effective pixels, so you should apply the DPI scale factor
		const UINT dpi = GetDpiForWindow(hwnd);
		const float scalingFactor = static_cast<float>(dpi) / 96;
		const int widthScaled = static_cast<int>(width * scalingFactor);
		const int heightScaled = static_cast<int>(height * scalingFactor);

		SetWindowPos(hwnd, nullptr, 0, 0, widthScaled, heightScaled, SWP_NOMOVE | SWP_NOZORDER);
	}

	void MainWindow::PlacementCenterWindowInMonitorWin32(HWND hwnd)
	{
		RECT windowMontiorRectToAdjust;
		GetWindowRect(hwnd, &windowMontiorRectToAdjust);
		ClipOrCenterRectToMonitorWin32(windowMontiorRectToAdjust);
		SetWindowPos(hwnd, nullptr, windowMontiorRectToAdjust.left,
			windowMontiorRectToAdjust.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	void MainWindow::ClipOrCenterRectToMonitorWin32(RECT& adjustedWindowRect)
	{
		MONITORINFO mi{ sizeof(mi) };
		GetMonitorInfoW(MonitorFromRect(&adjustedWindowRect, MONITOR_DEFAULTTONEAREST), &mi);

		const auto& rcWork = mi.rcWork;
		const int w = adjustedWindowRect.right - adjustedWindowRect.left;
		const int h = adjustedWindowRect.bottom - adjustedWindowRect.top;

		adjustedWindowRect.left = rcWork.left + (rcWork.right - rcWork.left - w) / 2;
		adjustedWindowRect.top = rcWork.top + (rcWork.bottom - rcWork.top - h) / 2;
		adjustedWindowRect.right = adjustedWindowRect.left + w;
		adjustedWindowRect.bottom = adjustedWindowRect.top + h;
	}
}
