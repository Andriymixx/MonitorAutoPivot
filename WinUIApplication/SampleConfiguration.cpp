#pragma once
#include "pch.h"
#include "SampleConfiguration.h"
#include "MainPage.xaml.h" 

namespace winrt
{
	using namespace Microsoft::UI::Xaml;
	using namespace Windows::Foundation::Collections;
}
namespace winrt::WinUIApplication
{

	IVector<Scenario> implementation::MainPage::scenariosInner = single_threaded_observable_vector<Scenario>(
		{
			Scenario{ L"Home", hstring(name_of<WinUIApplication::Page_Home>()), L"\uE80F"},
			Scenario{ L"Manage monitors", hstring(name_of<WinUIApplication::Page_ManageMonitors>()), L"\uE7F4"},
			Scenario{ L"Connection", hstring(name_of<WinUIApplication::Page_Connection>()), L"\uECF0"}
		});

	hstring SampleConfig::FeatureName{ L"MonitorAutoPivot" };
	ElementTheme SampleConfig::CurrentTheme{ ElementTheme::Default };
}
