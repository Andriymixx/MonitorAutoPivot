#pragma once
#include <winrt/Windows.Storage.Streams.h>

namespace SerialUtil
{
    winrt::Windows::Foundation::IAsyncOperation<bool> WaitForArduinoReadyAsync( uint32_t timeoutMillis);
}
