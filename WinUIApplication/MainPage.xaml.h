#pragma once
#include "MainPage.g.h"

namespace winrt::WinUIApplication::implementation
{
    struct MainWindow;
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        // For other pages to use NotifyUser from current var
        static WinUIApplication::MainPage Current() { return current; }
        static WinUIApplication::MainWindow GetMainWindow() { return mainWindow; }
        static WinUIApplication::MainPage current; 
        
        static Windows::Foundation::Collections::IVector<WinUIApplication::Scenario> Scenarios() { return scenariosInner; }
        void NotifyUser(hstring const& strMessage, Microsoft::UI::Xaml::Controls::InfoBarSeverity const& severity);
        void UpdateStatus(hstring const& strMessage, Microsoft::UI::Xaml::Controls::InfoBarSeverity severity);
        void NavView_Loaded(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void NavView_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView const& sender, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const& args);
        void NavView_Navigate(hstring navItemTag, Microsoft::UI::Xaml::Media::Animation::NavigationTransitionInfo const& transitionInfo);
        void NavView_BackRequested(Microsoft::UI::Xaml::Controls::NavigationView const& sender, Microsoft::UI::Xaml::Controls::NavigationViewBackRequestedEventArgs const& args);
        void ContentFrame_Navigated(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);
        void NavigateToPage(winrt::Windows::UI::Xaml::Interop::TypeName const& pageType);
        void SetMainWindow(WinUIApplication::MainWindow const& window);
        void EnableSaveButton();
        void SaveConfigButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
    private:
        static Windows::Foundation::Collections::IVector<Scenario> scenariosInner;
        /* Flag for keeping Save button enabled after doing multiple
           changes that were blocking ui elements */
        bool hasUnsavedChanges{ false };
        static WinUIApplication::MainWindow mainWindow;
    };
}

namespace winrt::WinUIApplication::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
