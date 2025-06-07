#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <windows.h>

namespace SerialUtil
{
    winrt::Windows::Foundation::IAsyncOperation<bool> WaitForArduinoReadyAsync( uint32_t timeoutMillis);
}
