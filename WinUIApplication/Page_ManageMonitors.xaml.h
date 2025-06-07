#pragma once

#include "Page_ManageMonitors.g.h"
#include "Monitor_Manager.h"
#include "Viewm_Preset.h"

namespace winrt::WinUIApplication::implementation
{
    struct Page_ManageMonitors : Page_ManageMonitorsT<Page_ManageMonitors>
    {
        Page_ManageMonitors();
        void RefreshMonitorsList();
        void UpdateSelectedMonitorDisplay();
        void RefreshButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void SelectButton_Click(winrt::Windows::Foundation::IInspectable const& sender,  winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void MonitorsListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender,  winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void LayoutListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender,  winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void AddButton_Click(winrt::Windows::Foundation::IInspectable const& sender,  winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void DeleteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void LoadPresets();

    private:
        static WinUIApplication::MainPage rootPage;
        std::vector<MonitorInfo> m_monitors;
        Windows::Foundation::Collections::IVector<WinUIApplication::Viewm_MonitorInfo> m_monitorViewModels{ 
            winrt::single_threaded_observable_vector<WinUIApplication::Viewm_MonitorInfo>() };
        Windows::Foundation::Collections::IVector<WinUIApplication::Viewm_Preset> m_presets{
            winrt::single_threaded_observable_vector<WinUIApplication::Viewm_Preset>() };
        // indexes for select buttons
        int m_selectedIndex = -1;
        int l_selectedIndex = -1;
    };
}

namespace winrt::WinUIApplication::factory_implementation
{
    struct Page_ManageMonitors : Page_ManageMonitorsT<Page_ManageMonitors, implementation::Page_ManageMonitors>
    {
    };
}
