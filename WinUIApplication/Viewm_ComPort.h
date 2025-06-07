#pragma once
#include "Viewm_ComPort.g.h"
// View model to display COM-port list element in 
namespace winrt::WinUIApplication::implementation
{
	struct Viewm_ComPort : Viewm_ComPortT<Viewm_ComPort>
	{
		Viewm_ComPort() = default;
		Viewm_ComPort(hstring const& name) : m_displayName(name) {}
		static WinUIApplication::Viewm_ComPort Create(hstring const& name)
		{
			return winrt::make<Viewm_ComPort>(name);
		}
		hstring DisplayName() const { return m_displayName; }
		void DisplayName(hstring const& value) { m_displayName = value; }
	private:
		hstring m_displayName;
	};
}
namespace winrt::WinUIApplication::factory_implementation
{
	struct Viewm_ComPort : Viewm_ComPortT<Viewm_ComPort, implementation::Viewm_ComPort> {};
}
