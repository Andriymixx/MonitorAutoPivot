#include "pch.h"
#include "Page_AxisRemapping.xaml.h"
#if __has_include("Page_AxisRemapping.g.cpp")
#include "Page_AxisRemapping.g.cpp"
#endif
#include <sstream>
#include <util.h>

bool hasNewAxisRemap = false;
namespace winrt
{
	using namespace Microsoft::UI::Xaml;
	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml::Controls;
}

namespace winrt::WinUIApplication::implementation
{
	WinUIApplication::MainPage Page_AxisRemapping::rootPage{ nullptr };
	Page_AxisRemapping::Page_AxisRemapping()
	{
		InitializeComponent();
		Page_AxisRemapping::rootPage = MainPage::Current();

	}

	void Page_AxisRemapping::OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&)
	{
		StartRemapping();
	}

	void Page_AxisRemapping::StartRemapping() {
		const char* remappingCommand = "REMAP_AXIS\n";
		DWORD bytesWritten;
		WriteFile(hSerial, remappingCommand, (DWORD)strlen(remappingCommand), &bytesWritten, NULL);
		FlushFileBuffers(hSerial);
		// Flags for running process of remapping
		runFlag = true;
		remappingDone = false;
		remappingError = false;

		// Thread for axis remapping
		readerThread = std::thread([this]() {
			std::function<void(const std::string&)> onData = [this](const std::string& data) {
				if (data.empty()) return;
				// reading till '\r'
				std::string cleanData = data;
				if (cleanData.back() == '\r') cleanData.pop_back();
				if (cleanData.empty()) return;

				std::wstring wdata = ConvertUTF8ToWString(cleanData);
				// Display received instruction
				if (cleanData.find("[Instruction]") != std::string::npos) {
					auto text = wdata.substr(wdata.find(L"[Instruction]") + 14);
					DispatcherQueue().TryEnqueue([this, text]() {
						InstructionTextBlock().Text(text);
						});

				}
				// Display received DATA
				else if (cleanData.find("[DATA]") != std::string::npos) {
					DispatcherQueue().TryEnqueue([this, wdata]() {
						DataTextBlock().Text(wdata);
						});
				}
				// Display received error
				else if (cleanData.find("[REMAPPING_ERR]") != std::string::npos) {
					auto result = wdata.substr(wdata.find(L"[REMAPPING_ERR]") + 16);
					remappingError = true;
					runFlag = false; // Stop receiving data from Arduino
					DispatcherQueue().TryEnqueue([this, result]() {
						DataTextBlock().Text(result);
						OkButton().Content(box_value(L"Restart"));
						CancelButton().Content(box_value(L"Exit"));
						});
				}
				// Reampping success
				else if (cleanData.find("REMAPPING_DONE") != std::string::npos) {
					remappingDone = true;
					runFlag = false; // Stop receiving data from Arduino
					// Parse values to remap axis 
					std::wstring result = wdata;
					size_t slashPos = result.find(L"/");
					std::wstring valuesStr = result.substr(slashPos);
					std::wistringstream iss(valuesStr);
					std::wstring token;
					std::vector<int> values;
					while (std::getline(iss, token, L'/')) {
						try {
							values.push_back(std::stoi(token));
						}
						catch (...) {}
					}
					// Save values to global variables 
					if (values.size() == 6) {
						ConfigUtil::g_remapConfig.mapX = values[0];
						ConfigUtil::g_remapConfig.mapY = values[1];
						ConfigUtil::g_remapConfig.mapZ = values[2];
						ConfigUtil::g_remapConfig.signX = values[3];
						ConfigUtil::g_remapConfig.signY = values[4];
						ConfigUtil::g_remapConfig.signZ = values[5];
					}
					hasNewAxisRemap = true; // Set flag to enable Save button later
					DispatcherQueue().TryEnqueue([this]() {
						DataTextBlock().Text(L"Remapping ended");
						OkButton().IsEnabled(false);
						CancelButton().Content(box_value(L"Exit"));
						});
				}
				else {
					DispatcherQueue().TryEnqueue([this, wdata]() {
						MessageTextBlock().Text(wdata);
						});
				}
				};
			SerialDataReaderThread(onData, std::ref(runFlag));
			});
	}

	void Page_AxisRemapping::RestartRemapping()
	{
		// End reader thread
		runFlag = false;
		if (readerThread.joinable()) readerThread.join();
		// Reset flags
		remappingDone = false;
		remappingError = false;

		// Clear UI
		InstructionTextBlock().Text(L"");
		DataTextBlock().Text(L"");
		MessageTextBlock().Text(L"");

		OkButton().Content(box_value(L"OK"));
		OkButton().IsEnabled(true);
		CancelButton().Content(box_value(L"Cancel"));


		// Start again
		StartRemapping();
	}

	void Page_AxisRemapping::OkButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (hSerial == NULL || hSerial == INVALID_HANDLE_VALUE) {
			rootPage.NotifyUser(L"COM-port is not open", InfoBarSeverity::Error);
			CancelButton().Content(box_value(L"Exit"));
			OkButton().IsEnabled(false);
			return;
		}
		// If remapping error this button acts like restart button
		if (remappingError) {
			RestartRemapping();
			return;
		}
		const char* okCommand = "OK\n";
		DWORD bytesWritten;
		WriteFile(hSerial, okCommand, (DWORD)strlen(okCommand), &bytesWritten, NULL);
		FlushFileBuffers(hSerial);
	}

	void Page_AxisRemapping::CancelButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		// Cancel and exit 
		if (!remappingDone && !remappingError) {
			const char* stopCommand = "STOP\n";
			DWORD written;
			WriteFile(hSerial, stopCommand, (DWORD)strlen(stopCommand), &written, NULL);
			FlushFileBuffers(hSerial);
		}
		else {
			runFlag = false;
			if (readerThread.joinable()) {
				if (std::this_thread::get_id() != readerThread.get_id()) {
					readerThread.join();
				}
				else {
					readerThread.detach();
				}
			}
			// if remapping succeeded Enable save button in UI
			if (hasNewAxisRemap) {
				rootPage.EnableSaveButton();
			}
			// Return to previous page
			if (Frame().CanGoBack())
				Frame().GoBack();
		}
	}
	// Open Windows settings to arrange display layout
	void Page_AxisRemapping::OnOpenWinDisplaySettings(IInspectable const&, RoutedEventArgs const&)
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
