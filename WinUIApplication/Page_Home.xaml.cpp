#include "pch.h"
#include "Page_Home.xaml.h"
#if __has_include("Page_Home.g.cpp")
#include "Page_Home.g.cpp"
#endif

namespace winrt
{
    using namespace Microsoft::UI::Xaml;
}

namespace winrt::WinUIApplication::implementation
{
    Page_Home::Page_Home()
    {
        InitializeComponent();
    }
}
