#include "pch.h"

#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include <g_flags.h>
#include <util.h>
#include <monitor_manager.h>
#include <SerialUtil.h>
#include <MainPage.xaml.h>

#include <wintoastlib.h>
#include <toast_utils.h>
#include <windows.h>
#include <shellapi.h>
// Define mutex for single running app
#define APP_MUTEX_NAME L"UniqueMonitorAutoPivotMutex"
HANDLE g_mutex = nullptr;

// Initialize global variables 
HANDLE hSerial = NULL;
bool wasConnected = false;
std::atomic<bool> stopMonitorWatcher = false;
bool startMinimized = false;

namespace winrt
{
	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
}
using namespace WinToastLib;
namespace winrt::WinUIApplication::implementation
{
	WinUIApplication::MainPage App::rootPage{ nullptr };
	App::App()
	{
		InitializeComponent();
		App::rootPage = MainPage::Current();

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
		UnhandledException([](winrt::IInspectable const&, winrt::UnhandledExceptionEventArgs const& e)
			{
				if (IsDebuggerPresent())
				{
					auto errorMessage = e.Message();
					__debugbreak();
				}
			});
#endif
	}
	App::~App()
	{
	}

	void App::OnLaunched(winrt::LaunchActivatedEventArgs const& args)
	{
		// Do not launch another copy of app if already runnig
		g_mutex = CreateMutexW(nullptr, TRUE, APP_MUTEX_NAME);
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			MessageBoxW(nullptr, L"App is already open", L"WinUIApplication",
				MB_OK | MB_ICONINFORMATION);
			ExitProcess(0);
		}
		// Reading arguments passed with launch
		int argc = 0;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

		if (argv)
		{
			for (int i = 0; i < argc; ++i)
			{
				// Set flag to launch minimized
				if (std::wcscmp(argv[i], L"--minimized") == 0)
				{
					startMinimized = true;
					break;
				}
			}

			LocalFree(argv);
		}
		window = winrt::make<MainWindow>();

		if (startMinimized)
		{
			// Minimize window
			auto impl = winrt::get_self<winrt::WinUIApplication::implementation::MainWindow>(window);
			HWND hwnd = impl->GetWindowHandle();
			ShowWindow(hwnd, SW_HIDE);
		}
		else
		{
			window.Activate();
		}
		// Initializing WinToast notifications
		ToastUtils::initialize();
		// Processing to other initialization of app in async mode 
		InitializeAppAsync();
	}

	winrt::Windows::Foundation::IAsyncAction App::InitializeAppAsync()
	{
		// Loading config
		if (LoadConfigJson())
		{
			hSerial = openSerialPort(ConfigUtil::selectedPort);
			if (hSerial != INVALID_HANDLE_VALUE)
			{
				rootPage.NotifyUser(L"Waiting Arduino...", InfoBarSeverity::Informational);
				// Waiting for Arduino to receive "READY" command in Async mode to continue 
				bool ready = co_await SerialUtil::WaitForArduinoReadyAsync(10000);
				if (ready)
				{
					SendAutoRemapConfig(ConfigUtil::g_remapConfig);
					rootPage.NotifyUser(L"Arduino is ready", InfoBarSeverity::Success);
				}
				else
				{
					std::wstring message = L"Arduino did not respond";
					rootPage.NotifyUser(L"Waiting timeout." + winrt::hstring(message), InfoBarSeverity::Error);

					WinToastTemplate templ(WinToastTemplate::Text02);
					templ.setTextField(L"Waiting timeout", WinToastTemplate::FirstLine);
					templ.setTextField(message, WinToastTemplate::SecondLine);
					templ.setDuration(WinToastTemplate::Duration::Short);
					/*templ.setExpiration(11000)*/
					ToastUtils::push(templ);
				}
			}
			else
			{
				std::wstring message = L"Could not open port: " + ConfigUtil::selectedPort;
				rootPage.NotifyUser(winrt::hstring(message), InfoBarSeverity::Error);

				WinToastTemplate templ1(WinToastTemplate::Text02);
				templ1.setTextField(L"Error", WinToastTemplate::FirstLine);
				templ1.setTextField(message, WinToastTemplate::SecondLine);
				templ1.setDuration(WinToastTemplate::Duration::Short);
				ToastUtils::push(templ1);
			}
		}
		else
		{
			rootPage.NotifyUser(L"Config did not find or could not load", InfoBarSeverity::Error);
		}
		// Start monitoring changes in active monitors
		startMonitorWatcher();
	}
}

