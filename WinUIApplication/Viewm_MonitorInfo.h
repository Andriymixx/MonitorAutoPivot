#pragma once
#include "Viewm_MonitorInfo.g.h"
// View model to display active monitor list element in UI
namespace winrt::WinUIApplication::implementation
{
	struct Viewm_MonitorInfo : Viewm_MonitorInfoT<Viewm_MonitorInfo>
	{
		Viewm_MonitorInfo() = default;
		Viewm_MonitorInfo(int32_t index, hstring const& deviceName, hstring const& displayNum, hstring const& gpuName)
			: m_index(index), m_deviceName(deviceName), m_displayNum(displayNum), m_gpuName(gpuName)
		{
		}
		static WinUIApplication::Viewm_MonitorInfo Create(int32_t index, hstring const& deviceName, hstring const& displayNum, hstring const& gpuName)
		{
			return winrt::make<Viewm_MonitorInfo>(index, deviceName, displayNum, gpuName);
		}

		int32_t Index() const { return m_index; }
		void Index(int32_t value) { m_index = value; }

		hstring DeviceName() const { return m_deviceName; }
		void DeviceName(hstring const& value) { m_deviceName = value; }

		hstring DisplayNum() const { return m_displayNum; }
		void DisplayNum(hstring const& value) { m_displayNum = value; }

		hstring GpuName() const { return m_gpuName; }
		void GpuName(hstring const& value) { m_gpuName = value; }

	private:
		int32_t m_index;
		hstring m_deviceName;
		hstring m_displayNum;
		hstring m_gpuName;
	};
}
namespace winrt::WinUIApplication::factory_implementation
{
	struct Viewm_MonitorInfo : Viewm_MonitorInfoT<Viewm_MonitorInfo, implementation::Viewm_MonitorInfo>
	{
	};
}
