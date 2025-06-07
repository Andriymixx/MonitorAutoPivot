#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif
#include "g_flags.h"

namespace winrt
{
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
	using namespace Microsoft::UI::Xaml::Media;
	using namespace Microsoft::UI::Xaml::Media::Animation;
	using namespace Microsoft::UI::Xaml::Navigation;
	using namespace Windows::UI::Xaml::Interop;
}

namespace winrt::WinUIApplication::implementation
{
	WinUIApplication::MainPage WinUIApplication::implementation::MainPage::current{ nullptr };
	WinUIApplication::MainWindow MainPage::mainWindow{ nullptr };
	MainPage::MainPage()
	{
		InitializeComponent();
		MainPage::current = *this;
	}
	void MainPage::SetMainWindow(WinUIApplication::MainWindow const& window)
	{
		mainWindow = window;
	}
	void MainPage::NavigateToPage(winrt::Windows::UI::Xaml::Interop::TypeName const& pageType)
	{
		if (ContentFrame())
		{
			ContentFrame().Navigate(pageType);
		}
	}
	void MainPage::NotifyUser(hstring const& strMessage, InfoBarSeverity const& severity)
	{
		// If called from the UI thread, then update immediately.
		// Otherwise, schedule a task on the UI thread to perform the update.
		if (this->DispatcherQueue().HasThreadAccess())
		{
			UpdateStatus(strMessage, severity);
		}
		else
		{
			DispatcherQueue().TryEnqueue([strongThis = get_strong(), this, strMessage, severity]
				{ UpdateStatus(strMessage, severity); });
		}
	}

	void MainPage::UpdateStatus(hstring const& strMessage, InfoBarSeverity severity)
	{
		infoBar().Message(strMessage);
		infoBar().IsOpen(!strMessage.empty());
		infoBar().Severity(severity);
	}

	void MainPage::NavView_Loaded(IInspectable const&, RoutedEventArgs const&)
	{
		for (auto&& s : Scenarios())
		{
			FontIcon fontIcon{};
			fontIcon.FontFamily(winrt::FontFamily(L"Segoe MDL2 Assets"));
			fontIcon.Glyph(s.IconGlyph);

			NavigationViewItem navViewItem{};
			navViewItem.Content(box_value(s.Title));
			navViewItem.Tag(box_value(s.ClassName));
			navViewItem.Icon(fontIcon);
			NavView().MenuItems().Append(navViewItem);
		}

		// NavView doesn't load any page by default, so load home page.
		NavView().SelectedItem(NavView().MenuItems().GetAt(0));

		// If navigation occurs on SelectionChanged, this isn't needed.
		// Because we use ItemInvoked to navigate, we need to call Navigate
		// here to load the home page.
		if (Scenarios().Size() > 0)
		{
			NavView_Navigate(Scenarios().GetAt(0).ClassName, nullptr);
		}
	}



	void MainPage::NavView_ItemInvoked(NavigationView const&, NavigationViewItemInvokedEventArgs const& args)
	{
		if (args.IsSettingsInvoked() == true)
		{
			hstring xamltypename = winrt::xaml_typename<SettingsPage>().Name;
			NavView_Navigate(xamltypename, args.RecommendedNavigationTransitionInfo());
		}
		else if (args.InvokedItemContainer() != nullptr)
		{
			auto navItemTag = winrt::unbox_value<hstring>(args.InvokedItemContainer().Tag());
			if (navItemTag != L"")
			{
				NavView_Navigate(navItemTag, args.RecommendedNavigationTransitionInfo());
			}
		}
	}
	void MainPage::EnableSaveButton()
	{
		// Enable button if user made changes that can be saved in config file
		SaveConfigButton().IsEnabled(true);
		hasUnsavedChanges = true;
	}
	void MainPage::SaveConfigButton_Tapped(IInspectable const&, Input::TappedRoutedEventArgs const&)
	{
		// Saving changes into a config file
		if (SaveConfigJson())
		{
			// Also creating reg key to autostart app
			NotifyUser(L"Changes saved to a config file successfully", InfoBarSeverity::Success);
			wchar_t exePath[MAX_PATH];
			GetModuleFileNameW(NULL, exePath, MAX_PATH);

			// Open reg key 
			std::wstring command = L"\"";
			command += exePath;
			command += L"\" --minimized";

			HKEY hKey;
			const wchar_t* runKeyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
			const wchar_t* valueName = L"MonitorAutoPivot";

			if (RegOpenKeyExW(HKEY_CURRENT_USER, runKeyPath, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
			{
				if (ConfigUtil::g_startWithWindows)
				{
					// Set reg key to enable autostartup
					RegSetValueExW(hKey, valueName, 0, REG_SZ, 
						reinterpret_cast<const BYTE*>(command.c_str()),
						static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
				}
				else
				{
					RegDeleteValueW(hKey, valueName);
				}
				RegCloseKey(hKey);

			}
		}
		else
		{
			NotifyUser(L"Config saved successfully", InfoBarSeverity::Success);
		}
		// Disable save button
		SaveConfigButton().IsEnabled(false);
		hasUnsavedChanges = false;
		
	}

	void MainPage::NavView_Navigate(hstring navItemTag, NavigationTransitionInfo const&)
	{
		TypeName pageType;

		if (navItemTag == winrt::xaml_typename<SettingsPage>().Name)
		{
			pageType = winrt::xaml_typename<SettingsPage>();
		}
		else
		{
			pageType.Name = navItemTag;
			pageType.Kind = TypeKind::Metadata;
		}

		// Get the page type before navigation so you can prevent duplicate
		// entries in the backstack.
		TypeName prePageType = ContentFrame().CurrentSourcePageType();

		// Only navigate if the selected page isn't currently loaded.
		if (prePageType.Name != pageType.Name)
		{
			DrillInNavigationTransitionInfo drillIn;
			ContentFrame().Navigate(pageType, nullptr, drillIn);
		}
	}

	void MainPage::NavView_BackRequested(NavigationView const&, NavigationViewBackRequestedEventArgs const&)
	{
		if (ContentFrame().CanGoBack())
		{
			ContentFrame().GoBack();
		}
	}

	void MainPage::ContentFrame_Navigated(IInspectable const&, NavigationEventArgs const& e)
	{
		// If on one of AxisRemapping or MakingLayout pages 
		bool isBlockingPage = 
			e.SourcePageType().Name == winrt::xaml_typename<Page_AxisRemapping>().Name ||
			e.SourcePageType().Name == winrt::xaml_typename<Page_MakingLayout>().Name;
		// Then blocking navigation back action
		NavView().IsBackEnabled(ContentFrame().CanGoBack() && !isBlockingPage);
		// Blocking only menu elements of NavigationView
		for (auto&& item : NavView().MenuItems())
		{
			auto navViewItem = item.try_as<NavigationViewItem>();
			if (navViewItem)
			{
				navViewItem.IsEnabled(!isBlockingPage);
			}
		}
		// Also blocking SettingsItem
		if (auto settingsItem = NavView().SettingsItem().try_as<NavigationViewItem>())
		{
			settingsItem.IsEnabled(!isBlockingPage);
		}
		// Also blocking save button
		if (hasUnsavedChanges)
		{
			SaveConfigButton().IsEnabled((hasUnsavedChanges && !isBlockingPage));
		}
		if (ContentFrame().SourcePageType().Name == winrt::xaml_typename<SettingsPage>().Name)
		{
			NavView().SelectedItem((NavView().SettingsItem().as<NavigationViewItem>()));
			NavView().Header(winrt::box_value(L"Settings"));
		}
		// Ordinary usage
		else if (!isBlockingPage)
		{
			for (auto&& item : NavView().MenuItems())
			{
				auto navViewItem = item.try_as<NavigationViewItem>();
				if (navViewItem != nullptr &&
					winrt::unbox_value<hstring>(navViewItem.Tag()) == e.SourcePageType().Name)
				{
					NavView().SelectedItem(navViewItem);
					NavView().Header(navViewItem.Content());
					break;
				}
			}
		}
	}

}
