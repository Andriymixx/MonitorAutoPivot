#include "pch.h"
#include "SettingsPage.xaml.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif
#include <SampleConfiguration.h>
#include <g_flags.h>
#include <MainPage.xaml.h>
namespace winrt
{
    using namespace Microsoft::UI::Xaml;
    using namespace Microsoft::UI::Xaml::Controls;
    using namespace Microsoft::UI::Xaml::Navigation;
    using namespace Windows::Foundation;
}

namespace winrt::WinUIApplication::implementation
{
    WinUIApplication::MainPage SettingsPage::rootPage{ nullptr };
    SettingsPage::SettingsPage()
    {
        InitializeComponent();
        SettingsPage::rootPage = MainPage::Current();
        m_isInitializing = true;
        // turn into state from config 
        MinimizeOnCloseToggle().IsOn(ConfigUtil::g_minimizeOnClose);
        StartWithWindowsToggle().IsOn(ConfigUtil::g_startWithWindows);
        SampleConfig::CurrentTheme = static_cast<ElementTheme>(ConfigUtil::g_selectedTheme);
        m_isInitializing = false;
    }

    void SettingsPage::OnNavigatedTo(NavigationEventArgs const&)
    {
        // Not enabling Save button when navigated to setting page 
        // and ckecking radio button automatically 
        m_isInitializing = true;
        for (UIElement&& c : themePanel().Children())
        {
            auto tag = c.as<RadioButton>().Tag().as<ElementTheme>();
            if (tag == SampleConfig::CurrentTheme)
            {
                auto radioButton = c.as<RadioButton>();
                radioButton.IsChecked(true);
            }
        }
        m_isInitializing = false;
    }

    void SettingsPage::OnThemeRadioButtonChecked(IInspectable const& sender, RoutedEventArgs const&)
    { 
        // Theme managing
        if (m_isInitializing) return;
        auto radioButton = sender.as<RadioButton>();
        auto selectedTheme = unbox_value<ElementTheme>(radioButton.Tag());
        auto content = MainPage::Current().Content().as<Grid>();
        if (content != nullptr)
        {
            content.RequestedTheme(selectedTheme);
            SampleConfig::CurrentTheme = content.RequestedTheme();
        }  
        ConfigUtil::g_selectedTheme = static_cast<int>(selectedTheme);
        rootPage.EnableSaveButton();
    }

    void SettingsPage::MinimizeOnCloseToggle_Toggled(IInspectable const& sender, RoutedEventArgs const&)
    {
        if (m_isInitializing) return;
        auto toggle = sender.as<ToggleSwitch>();
        ConfigUtil::g_minimizeOnClose = toggle.IsOn();
        rootPage.EnableSaveButton();
    }

    void SettingsPage::StartWithWindowsToggle_Toggled(IInspectable const& sender, RoutedEventArgs const&)
    {
        if (m_isInitializing) return;
        auto toggle = sender.as<ToggleSwitch>();
        ConfigUtil::g_startWithWindows = toggle.IsOn();
        rootPage.EnableSaveButton();
    }
}
