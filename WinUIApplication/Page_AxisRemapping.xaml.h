#pragma once

#include "Page_AxisRemapping.g.h"
#include <thread>
#include <atomic>
namespace winrt::WinUIApplication::implementation
{
    struct Page_AxisRemapping : Page_AxisRemappingT<Page_AxisRemapping>
    {
        Page_AxisRemapping();
        void OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&);
        void OkButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void CancelButton_Click(winrt::Windows::Foundation::IInspectable const&,  winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnOpenWinDisplaySettings(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::UI::Xaml::DispatcherTimer statusTimer{ nullptr };
    private:
        void StartRemapping();
        void RestartRemapping();
        std::thread readerThread;
        std::atomic<bool> runFlag = false;
        std::atomic<bool> remappingError { false };
        std::atomic<bool> remappingDone{ false };
        static WinUIApplication::MainPage rootPage;
    };
}
namespace winrt::WinUIApplication::factory_implementation
{
    struct Page_AxisRemapping : Page_AxisRemappingT<Page_AxisRemapping, implementation::Page_AxisRemapping>
    {
    };
}
