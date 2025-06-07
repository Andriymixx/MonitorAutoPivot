#pragma once
#include "Viewm_Preset.g.h"
// View model to display preset list element in UI
namespace winrt::WinUIApplication::implementation
{
	struct Viewm_Preset : Viewm_PresetT<Viewm_Preset>
	{
		Viewm_Preset() = default;
		Viewm_Preset(hstring const& key, hstring const& displayName)
			: m_key(key), m_displayName(displayName) {
		}
		static WinUIApplication::Viewm_Preset Create(hstring const& key, hstring const& displayName)
		{
			return winrt::make<WinUIApplication::implementation::Viewm_Preset>(key, displayName);
		}

		hstring Key() const { return m_key; }
		void Key(hstring const& value) { m_key = value; }

		hstring DisplayName() const
		{
			// return as one monitor per line
			std::wstring name = m_displayName.c_str();
			size_t plusPos = name.find(L'+');
			if (plusPos != std::wstring::npos)
			{
				name.replace(plusPos, 1, L"\n");
			}
			return hstring(name);
		}
		void DisplayName(hstring const& value) { m_displayName = value; }

	private:
		hstring m_key;
		hstring m_displayName;
	};
}
namespace winrt::WinUIApplication::factory_implementation
{
	struct Viewm_Preset : Viewm_PresetT<Viewm_Preset, implementation::Viewm_Preset>
	{
	};
}
