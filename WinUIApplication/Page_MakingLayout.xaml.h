#pragma once
#include "Page_MakingLayout.g.h"

namespace winrt::WinUIApplication::implementation
{
    struct Page_MakingLayout : Page_MakingLayoutT<Page_MakingLayout>
    {
        Page_MakingLayout();
        void OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);
        void OnSaveClick(winrt::Windows::Foundation::IInspectable const& sender,  winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnExitClick(winrt::Windows::Foundation::IInspectable const& sender,  winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnOpenWinDisplaySettings(winrt::Windows::Foundation::IInspectable const& sender,  winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void SavePreset(int orientation);
        void ShowOverwriteDialog(int orientation);
    private:
        void StartMakingLayout();
        std::thread readerThread;
        std::atomic<bool> runFlag = false;
        static WinUIApplication::MainPage rootPage;
    };
}

namespace winrt::WinUIApplication::factory_implementation
{
    struct Page_MakingLayout : Page_MakingLayoutT<Page_MakingLayout, implementation::Page_MakingLayout>
    {
    };
}

