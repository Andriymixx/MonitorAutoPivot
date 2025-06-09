
#include <nlohmann/json.hpp>
#include "monitor_manager.h"
#include "g_flags.h"
#include <fstream>
#include <shlobj.h>
#pragma comment(lib, "Shell32.lib")
using json = nlohmann::json;
namespace MonitorManager {
	MonitorInfo selectedMonitor;
}
namespace ConfigUtil {
	RemapConfig g_remapConfig;
	std::wstring selectedPort = L"";
	bool g_minimizeOnClose = false;
	bool g_startWithWindows = false;
	int g_selectedTheme = 0;
}
// get Path of config file in User Documents folder
std::string GetConfigPathInDocuments()
{
	char path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path)))
	{
		std::filesystem::path configPath = std::filesystem::path(path)
			/ "MonitorAutoPivot" / "config.json";
		return configPath.string();
	}
	return "";
}

bool SaveConfigJson() {
	json j;
	// converting wstring to string
	auto ws2s = [](const std::wstring& wstr)
		{
			if (wstr.empty()) return std::string();
			int size_needed = WideCharToMultiByte(CP_UTF8, 0,
				wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
			std::string strTo(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(),
				&strTo[0], size_needed, NULL, NULL);
			return strTo;
		};

	j["SETTINGS"]["selectedTheme"] = ConfigUtil::g_selectedTheme;
	j["SETTINGS"]["minimizeOnClose"] = ConfigUtil::g_minimizeOnClose;
	j["SETTINGS"]["startWithWindows"] = ConfigUtil::g_startWithWindows;
	j["COM_PORT"] = ws2s(ConfigUtil::selectedPort);
	j["ACC_REMAP"]["axisMapX"] = ConfigUtil::g_remapConfig.mapX;
	j["ACC_REMAP"]["axisMapY"] = ConfigUtil::g_remapConfig.mapY;
	j["ACC_REMAP"]["axisMapZ"] = ConfigUtil::g_remapConfig.mapZ;
	j["ACC_REMAP"]["axisSignX"] = ConfigUtil::g_remapConfig.signX;
	j["ACC_REMAP"]["axisSignY"] = ConfigUtil::g_remapConfig.signY;
	j["ACC_REMAP"]["axisSignZ"] = ConfigUtil::g_remapConfig.signZ;
	j["MONITOR"]["index"] = MonitorManager::selectedMonitor.index;
	j["MONITOR"]["monitorPathFromTarget"] = ws2s(MonitorManager::selectedMonitor.monitorPathFromTarget);
	j["MONITOR"]["displayNum"] = ws2s(MonitorManager::selectedMonitor.displayNum);
	j["MONITOR"]["deviceName"] = ws2s(MonitorManager::selectedMonitor.deviceName);
	j["MONITOR"]["gpuName"] = ws2s(MonitorManager::selectedMonitor.gpuName);

	
	json presetByMonSet = json::object();
	for (const auto& [configKey, ltPreset] : MonitorManager::PresetByMonitorSet) {
		json ltPresetJson = json::object();
		ltPresetJson["friendlyName"] = ws2s(ltPreset.friendlyName);
		json monitorsPositionsJson = json::object();
		for (const auto& [orientation, monPos] : ltPreset.layoutByOrientation) {
			json positionsArray = json::array();
			for (const auto& pos : monPos) {
				positionsArray.push_back({
					{"monitorPathFromTarget", ws2s(pos.monitorPathFromTarget)},
					{"x", pos.x},
					{"y", pos.y}
					});
			}
			monitorsPositionsJson[std::to_string(orientation)] = positionsArray;
		}
		ltPresetJson["LayoutPreset"] = monitorsPositionsJson;
		presetByMonSet[ws2s(configKey)] = ltPresetJson;
	}
	j["PRESET_BY_MONITOR_SET"] = presetByMonSet;

	// Saving to specified path in documents folder 
	std::ofstream file(GetConfigPathInDocuments());
	if (!file.is_open()) return false;
	file << j.dump(4);
	file.close();
	return true;
}

bool LoadConfigJson() {
	std::ifstream file(GetConfigPathInDocuments());
	if (!file.is_open()) return false;
	json j;
	file >> j;
	file.close();
	// converting from string to wstring
	auto s2ws = [](const std::string& s) {
		return std::wstring(s.begin(), s.end());
		};
	try {
		ConfigUtil::selectedPort = s2ws(j.at("COM_PORT").get<std::string>());
		ConfigUtil::g_selectedTheme = j["SETTINGS"].value("selectedTheme", 0);
		ConfigUtil::g_minimizeOnClose = j["SETTINGS"].value("minimizeOnClose", false);
		ConfigUtil::g_startWithWindows = j["SETTINGS"].value("startWithWindows", false);
		RemapConfig rc;
		rc.mapX = j["ACC_REMAP"]["axisMapX"];
		rc.mapY = j["ACC_REMAP"]["axisMapY"];
		rc.mapZ = j["ACC_REMAP"]["axisMapZ"];
		rc.signX = j["ACC_REMAP"]["axisSignX"];
		rc.signY = j["ACC_REMAP"]["axisSignY"];
		rc.signZ = j["ACC_REMAP"]["axisSignZ"];
		ConfigUtil::g_remapConfig = rc;
		MonitorInfo m;
		m.index = j["MONITOR"]["index"];
		m.monitorPathFromTarget = s2ws(j["MONITOR"]["monitorPathFromTarget"]);
		m.displayNum = s2ws(j["MONITOR"]["displayNum"]);
		m.deviceName = s2ws(j["MONITOR"]["deviceName"]);
		m.gpuName = s2ws(j["MONITOR"]["gpuName"]);
		MonitorManager::selectedMonitor = m;
		MonitorManager::PresetByMonitorSet.clear();
		auto presetByMonSetIt = j.find("PRESET_BY_MONITOR_SET");
		if (presetByMonSetIt != j.end()) {
			for (auto& [configKeyStr, ltPresetJson] : presetByMonSetIt->items()) {
				std::wstring configKey = s2ws(configKeyStr);
				LayoutPreset ltPreset;
				ltPreset.friendlyName = s2ws(ltPresetJson["friendlyName"].get<std::string>());
				if (ltPresetJson.contains("LayoutPreset")) {
					auto& monitorPositionsJson = ltPresetJson["LayoutPreset"];
					for (auto& [orientationStr, monPos] : monitorPositionsJson.items()) {
						int orientation = std::stoi(orientationStr);
						MonitorsPositions positionsArray;
						for (const auto& pos : monPos) {
							positionsArray.push_back({
								s2ws(pos["monitorPathFromTarget"]),
								pos["x"],
								pos["y"]
								});
						}
						ltPreset.layoutByOrientation[orientation] = positionsArray;
					}
				}
				MonitorManager::PresetByMonitorSet[configKey] = ltPreset;
			}
		}
		return true;
	}
	catch (...) {
		return false;
	}
}
