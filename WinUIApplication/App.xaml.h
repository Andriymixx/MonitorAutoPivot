#pragma once
#include "App.xaml.g.h"
#include "pch.h"

namespace winrt::WinUIApplication::implementation
{
    struct App : AppT<App>
    {
        App();
        ~App();
        winrt::Windows::Foundation::IAsyncAction InitializeAppAsync();
        
        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
        static WinUIApplication::MainPage rootPage;
    private:
        winrt::WinUIApplication::MainWindow window{ nullptr }; 
    };
}
