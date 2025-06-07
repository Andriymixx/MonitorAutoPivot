#include "pch.h"
#include "Page_Connection.xaml.h"
#if __has_include("Page_Connection.g.cpp")
#include "Page_Connection.g.cpp"
#endif
#include <config_util.h>
#include <util.h>
#include <MainPage.xaml.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <SerialUtil.h>
#include <wintoastlib.h>
#include <toast_utils.h>

namespace winrt
{
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
	using namespace Microsoft::UI::Xaml::Documents;
	using namespace Windows::Foundation;
	using namespace Windows::Foundation::Collections;
	using namespace Microsoft::UI::Xaml::Media;
}
using namespace WinToastLib;
namespace winrt::WinUIApplication::implementation
{
	winrt::Microsoft::UI::Xaml::DispatcherTimer statusTimer{ nullptr };
	WinUIApplication::MainPage Page_Connection::rootPage{ nullptr };
	Page_Connection::Page_Connection()
	{
		InitializeComponent();
		Page_Connection::rootPage = MainPage::Current();
		// Displaying state of connection
		if ((hSerial != NULL && hSerial != INVALID_HANDLE_VALUE)) {
			ComPortTitleText().Text(L"Already connected to: " + ConfigUtil::selectedPort);
		}
		else {
			ComPortTitleText().Text(L"Not connected");
		}
		// Timer to update the auto rotation ToggleSwitch
		statusTimer = Microsoft::UI::Xaml::DispatcherTimer();
		statusTimer.Interval(std::chrono::milliseconds(500));
		statusTimer.Tick({ this, &Page_Connection::OnStatusTimerTick });
		statusTimer.Start();
	}

	void Page_Connection::OnStatusTimerTick(IInspectable const&, IInspectable const&)
	{
		bool isActive = dataReceivingActive;
		if (RotationToggleSwitch().IsOn() != isActive) {
			RotationToggleSwitch().IsOn(isActive);
		}
	}

	void Page_Connection::RotationToggleSwitch_Toggled(IInspectable const& sender, RoutedEventArgs const&)
	{
		auto toggle = sender.as<ToggleSwitch>();
		if (toggle.IsOn())
		{
			// Check if connected
			if (hSerial == NULL || hSerial == INVALID_HANDLE_VALUE) {
				rootPage.NotifyUser(L"COM-port is not open", InfoBarSeverity::Error);
				toggle.IsOn(false); // toggle switch off
				return;
			}
			if (!dataReceivingActive)
				StartDataReceiving();
		}
		else
		{
			if (dataReceivingActive)
				StopDataReceiving();
		}
	}

	void Page_Connection::OnNavigatedFrom(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e)
	{
		if (statusTimer != nullptr)
			statusTimer.Stop();
	}

	void Page_Connection::RemapButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (hSerial == NULL || hSerial == INVALID_HANDLE_VALUE) {
			rootPage.NotifyUser(L"COM-port is not open", InfoBarSeverity::Error);
			return;
		}
		// Stop receiving data to so Arduino will be ready to switch to other state
		if (dataReceivingActive) {
			StopDataReceiving();
		}
		if (rootPage)
		{
			auto mainPage = winrt::get_self<winrt::WinUIApplication::implementation::MainPage>(rootPage);
			if (mainPage)
			{
				mainPage->NavigateToPage(winrt::xaml_typename<winrt::WinUIApplication::Page_AxisRemapping>());
			}
		}
	}

	void Page_Connection::RefreshButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		// Clear list of COM=-ports
		m_comPorts.Clear();
		auto ports = ListAvailableComPorts();
		if (ports.size() == 0) {
			ComPortTitleText().Text(L"Did not find any COM-Port");
		}
		for (const auto& port : ports) {
			m_comPorts.Append(WinUIApplication::Viewm_ComPort::Create(hstring(port)));
		}
		ComPortListView().ItemsSource(m_comPorts);
		ConnectButton().IsEnabled(false);
		ComPortListView().SelectionChanged({ this, &Page_Connection::ComPortListView_SelectionChanged });
	}

	void Page_Connection::ComPortListView_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
	{
		c_selectedIndex = ComPortListView().SelectedIndex();
		ConnectButton().IsEnabled(c_selectedIndex >= 0);
	}

	void Page_Connection::ConnectButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (c_selectedIndex >= 0 && c_selectedIndex < static_cast<int>(m_comPorts.Size())) {
			auto selected = m_comPorts.GetAt(c_selectedIndex).DisplayName();
			if (dataReceivingActive) {
				StopDataReceiving();
			}
			if (!(hSerial == NULL || hSerial == INVALID_HANDLE_VALUE)) {
				CloseHandle(hSerial);
				hSerial = NULL;
			}
			ConnectAsync();
			rootPage.EnableSaveButton();
		}
	}

	winrt::Windows::Foundation::IAsyncAction Page_Connection::ConnectAsync()
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
				ComPortTitleText().Text(L"Connected to: " + ConfigUtil::selectedPort);
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
		}
	}
}