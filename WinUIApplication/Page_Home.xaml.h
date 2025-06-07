#pragma once

#include "Page_Home.g.h"

namespace winrt::WinUIApplication::implementation
{
    struct Page_Home : Page_HomeT<Page_Home>
    {
        Page_Home();
    };
}

namespace winrt::WinUIApplication::factory_implementation
{
    struct Page_Home : Page_HomeT<Page_Home, implementation::Page_Home>
    {
    };
}

