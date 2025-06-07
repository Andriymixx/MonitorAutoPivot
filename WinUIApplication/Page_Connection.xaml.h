#pragma once

#include "Page_Connection.g.h"
#include "Viewm_ComPort.h"
namespace winrt::WinUIApplication::implementation
{
    struct Page_Connection : Page_ConnectionT<Page_Connection>
    {
        Page_Connection();
        void RemapButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnNavigatedFrom(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);
        void OnStatusTimerTick(IInspectable const&, IInspectable const&);
        void RotationToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ComPortListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void ConnectButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RefreshButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction ConnectAsync();
    private:
        static WinUIApplication::MainPage rootPage;
        int c_selectedIndex = -1;
        Windows::Foundation::Collections::IVector<WinUIApplication::Viewm_ComPort> m_comPorts{ winrt::single_threaded_observable_vector<WinUIApplication::Viewm_ComPort>() };
    };
}

namespace winrt::WinUIApplication::factory_implementation
{
    struct Page_Connection : Page_ConnectionT<Page_Connection, implementation::Page_Connection>
    {
    };
}
