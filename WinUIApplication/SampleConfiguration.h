#pragma once
#include "pch.h"

namespace winrt::WinUIApplication
{
    struct SampleConfig
    {
    public:
        static hstring FeatureName;
        static Microsoft::UI::Xaml::ElementTheme CurrentTheme;
    };
}
