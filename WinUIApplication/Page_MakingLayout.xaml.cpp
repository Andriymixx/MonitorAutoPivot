#include "pch.h"
#include "Page_MakingLayout.xaml.h"
#if __has_include("Page_MakingLayout.g.cpp")
#include "Page_MakingLayout.g.cpp"
#endif
#include "g_flags.h"
#include <thread>
#include <atomic>
#include "util.h"
#include <sstream>
#include "MainWindow.xaml.h"
#include "MainPage.xaml.h"
#include <monitor_manager.h>
#include <windows.h>
namespace winrt
{
	using namespace Microsoft::UI::Xaml;
	using namespace WinUIApplication;
	using namespace Microsoft::UI::Xaml::Controls;
}

bool hasNewPreset = false;
bool saveError = false;
namespace winrt::WinUIApplication::implementation
{
	WinUIApplication::MainPage Page_MakingLayout::rootPage{ nullptr };
	Page_MakingLayout::Page_MakingLayout()
	{
		InitializeComponent();
		Page_MakingLayout::rootPage = MainPage::Current();
	}

	void Page_MakingLayout::OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&)
	{
		StartMakingLayout();
	}

	void Page_MakingLayout::StartMakingLayout() {
		presetMakingActive = true;
		runFlag = true;
		const char* remapping = "SEND_DATA\n";
		DWORD bytesWritten;
		WriteFile(hSerial, remapping, (DWORD)strlen(remapping), &bytesWritten, NULL);
		readerThread = std::thread([this]() {
			std::function<void(const std::string&)> onData = [this](const std::string& data) {
				if (data.find("[DATA]") != std::string::npos) {
					std::wstring wdata = ConvertUTF8ToWString(data);
					std::wstring val = wdata.substr(wdata.find(L"[DATA]") + 7);
					// Parse received orientation from Arduino
					try {
						int orientation = std::stoi(val);
						// Saving current orientation
						MonitorManager::currentOrientation = orientation;
						if (orientation != -1) {
							setOrientation(MonitorManager::selectedMonitor, orientation);
						}
					}
					catch (...) {}
				}
				else if (data.find("[SENDING_STOPPED]") != std::string::npos) {
					runFlag = false;
				}
				};
			SerialDataReaderThread(onData, std::ref(runFlag));
			});
	}
	void Page_MakingLayout::OnSaveClick(IInspectable const&, RoutedEventArgs const&)
	{
		if (hSerial == NULL || hSerial == INVALID_HANDLE_VALUE) {
			rootPage.NotifyUser(L"COM-port is not open", InfoBarSeverity::Error);
			SaveButton().IsEnabled(false);
			saveError = true;
			return;
		}
		// Loading current orientation
		int orientationSnapshot = MonitorManager::currentOrientation.load();
		// Generation a key_id of active monitors 
		auto key = generateMonitorConfigKey(MonitorManager::activeMonitors);
		auto& ltPreset = MonitorManager::PresetByMonitorSet[key];
		// If for active monitors there is already saved layout ask for overwrite
		if (ltPreset.layoutByOrientation.find(orientationSnapshot) != ltPreset.layoutByOrientation.end()) {
			ShowOverwriteDialog(orientationSnapshot);
		}
		else {
			SavePreset(orientationSnapshot);
		}
	}

	void Page_MakingLayout::SavePreset(int orientation)
	{
		saveLayoutForOrientation(orientation);
		std::wstringstream ss;
		ss << L"Layout for " << orientation << L" saved";
		rootPage.NotifyUser(hstring(ss.str()), InfoBarSeverity::Informational);
		hasNewPreset = true;
	}

	void Page_MakingLayout::ShowOverwriteDialog(int orientation)
	{
		auto dialog = winrt::Microsoft::UI::Xaml::Controls::ContentDialog();
		dialog.XamlRoot(this->XamlRoot());
		dialog.Title(winrt::box_value(L"Overwrite preset?"));
		dialog.Content(winrt::box_value(L"For this orientation there is already saved preset. Overwrite it?"));
		dialog.PrimaryButtonText(L"Yes");
		dialog.CloseButtonText(L"No");
		dialog.DefaultButton(winrt::Microsoft::UI::Xaml::Controls::ContentDialogButton::Primary);

		auto self = get_strong();
		dialog.ShowAsync().Completed([self, orientation](auto&& asyncOp, winrt::Windows::Foundation::AsyncStatus status) {
			if (status == winrt::Windows::Foundation::AsyncStatus::Completed) {
				auto result = asyncOp.GetResults();
				if (result == winrt::Microsoft::UI::Xaml::Controls::ContentDialogResult::Primary) {
					self->SavePreset(orientation);
				}
				else {
					self->rootPage.NotifyUser(L"Preset saved without new changes", InfoBarSeverity::Error);
				}
			}
			});
	}

	void Page_MakingLayout::OnExitClick(IInspectable const&, RoutedEventArgs const&)
	{
		if (!saveError) {
			const char* stopCommand = "STOP\n";
			DWORD written;
			WriteFile(hSerial, stopCommand, (DWORD)strlen(stopCommand), &written, NULL);
			FlushFileBuffers(hSerial);
		}
		runFlag = false;
		if (readerThread.joinable()) {
			if (std::this_thread::get_id() != readerThread.get_id()) {
				readerThread.join();
			}
			else {
				readerThread.detach();
			}
		}

		presetMakingActive = false;
		if (hasNewPreset) {
			rootPage.EnableSaveButton();
		}
		// Go back to previous page
		if (Frame().CanGoBack()) {
			Frame().GoBack();
		}
		if (!saveError) {
			// Start data receiving 
			StartDataReceiving();
		}
	}

	// Open Windows settings to arrange display layout
	void Page_MakingLayout::OnOpenWinDisplaySettings(IInspectable const&, RoutedEventArgs const&)
	{
		ShellExecuteW(
			nullptr,
			L"open",
			L"ms-settings:display",
			nullptr,
			nullptr,
			SW_SHOWNORMAL
		);
	}
}
