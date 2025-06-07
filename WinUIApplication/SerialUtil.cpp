#include "pch.h"
#include "SerialUtil.h"
#include <chrono>
#include <thread>
#include "util.h"

using namespace winrt;
using namespace Windows::Foundation;

namespace SerialUtil
{
    IAsyncOperation<bool> WaitForArduinoReadyAsync(uint32_t timeoutMillis)
    {
        co_return co_await winrt::resume_background(), WaitForArduinoReady(timeoutMillis);
    }
}
