#include "pch.h"
#include "Page_ManageMonitors.xaml.h"
#if __has_include("Page_ManageMonitors.g.cpp")
#include "Page_ManageMonitors.g.cpp"
#endif

#include "gui.h"
#include <thread>
#include <chrono>
#include <map>
#include <g_flags.h>
#include <util.h>
#include "Viewm_Preset.h"
#include "Monitor_Manager.h"
#include "MainPage.xaml.h"
#include "MainWindow.xaml.h"

namespace winrt
{
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
	using namespace Microsoft::UI::Xaml::Documents;
	using namespace Windows::Foundation;
	using namespace Windows::Foundation::Collections;
	using namespace Microsoft::UI::Xaml::Media;
}

namespace winrt::WinUIApplication::implementation
{
	WinUIApplication::MainPage Page_ManageMonitors::rootPage{ nullptr };
	Page_ManageMonitors::Page_ManageMonitors()
	{
		InitializeComponent();
		Page_ManageMonitors::rootPage = MainPage::Current();
		UpdateSelectedMonitorDisplay();
		MonitorsListView().SelectionChanged({ this, &Page_ManageMonitors::MonitorsListView_SelectionChanged });
		LayoutListView().ItemsSource(m_presets);
		LayoutListView().SelectionChanged({ this, &Page_ManageMonitors::LayoutListView_SelectionChanged });
		LoadPresets();
	}

	void Page_ManageMonitors::AddButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		// To specify layout connection to Arduino is required
		if (hSerial == NULL || hSerial == INVALID_HANDLE_VALUE) {
			rootPage.NotifyUser(L"COM-Port is not open", InfoBarSeverity::Error);
			return;
		}
		// only multiple monitors using saved layout
		if (MonitorManager::monitors.size() == 1) {
			rootPage.NotifyUser(L"There is no need to specify layout for a SINGLE monitor", InfoBarSeverity::Informational);
			return;
		}
		if (dataReceivingActive) {
			StopDataReceiving();
		}
		// Navigate to page of specifying layout
		if (rootPage)
		{
			auto mainPage = winrt::get_self<winrt::WinUIApplication::implementation::MainPage>(rootPage);
			if (mainPage)
			{
				mainPage->NavigateToPage(winrt::xaml_typename<winrt::WinUIApplication::Page_MakingLayout>());
			}
		}
	}

	void Page_ManageMonitors::LoadPresets()
	{
		m_presets.Clear();
		if (MonitorManager::PresetByMonitorSet.empty())
		{
			return;
		}

		for (const auto& [configKey, configGroup] : MonitorManager::PresetByMonitorSet)
		{
			std::wstring display;
			// Displaying only saved presets with friendly monitor names
			if (!configGroup.friendlyName.empty()) {
				m_presets.Append(Viewm_Preset::Create(hstring(configKey), hstring(configGroup.friendlyName)));
			}
		}
	}

	void Page_ManageMonitors::LayoutListView_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
	{
		l_selectedIndex = LayoutListView().SelectedIndex();
		DeleteButton().IsEnabled(l_selectedIndex >= 0);
	}

	void Page_ManageMonitors::DeleteButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (l_selectedIndex >= 0 && l_selectedIndex < static_cast<int>(m_presets.Size()))
		{
			auto key = m_presets.GetAt(l_selectedIndex).Key();
			MonitorManager::PresetByMonitorSet.erase(key.c_str());
			LoadPresets();
			DeleteButton().IsEnabled(false);
			rootPage.EnableSaveButton();
			l_selectedIndex = -1;
		}
	}

	void Page_ManageMonitors::UpdateSelectedMonitorDisplay()
	{
		if (MonitorManager::selectedMonitor.displayNum.empty())
		{
			MonitorTitleText().Text(L"Monitor is not selected");
			DisplayNumberText().Text(L"");
			GpuNameText().Text(L"");
		}
		// Display selected monitor in its UI section
		else
		{
			MonitorTitleText().Text(MonitorManager::selectedMonitor.deviceName);
			DisplayNumberText().Text(MonitorManager::selectedMonitor.displayNum);
			GpuNameText().Text(MonitorManager::selectedMonitor.gpuName);
		}
	}

	void Page_ManageMonitors::SelectButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_monitors.size()))
		{
			// Saving selected monitor
			MonitorManager::selectedMonitor = m_monitors[m_selectedIndex];
			rootPage.EnableSaveButton();
			UpdateSelectedMonitorDisplay();
		}
	}

	void Page_ManageMonitors::MonitorsListView_SelectionChanged(
		IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&)
	{
		m_selectedIndex = MonitorsListView().SelectedIndex();
		SelectButton().IsEnabled(m_selectedIndex >= 0);
	}
	void Page_ManageMonitors::RefreshMonitorsList()
	{
		MonitorManager::monitors = listMonitors();
		m_monitors = MonitorManager::monitors;
		m_monitorViewModels.Clear();
		if (m_monitors.empty())
			return;
		// Listing active monitors in UI
		for (const auto& monitor : m_monitors)
		{
			m_monitorViewModels.Append(
				Viewm_MonitorInfo::Create(
					monitor.index,
					hstring(monitor.deviceName),
					hstring(monitor.displayNum),
					hstring(monitor.gpuName)
				)
			);
		}
		MonitorsListView().ItemsSource(m_monitorViewModels);
		// Displaying on each of monitors its index
		std::thread guiThread(RunWindowMessageLoopAsync, MonitorManager::monitors);
		guiThread.detach();
	}

	void Page_ManageMonitors::RefreshButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		RefreshMonitorsList();
	}
}
