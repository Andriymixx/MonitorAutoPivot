#ifndef MONITOR_MANAGER_H
#define MONITOR_MANAGER_H
#include <vector>
#include <string>
#include <Windows.h>
#include <map>
#include <atomic>

// Structure for Info about monitor
struct MonitorInfo {
	int index;
	std::wstring monitorPathFromTarget;
	std::wstring displayNum;
	std::wstring deviceName;
	std::wstring gpuName;

};

// Simplified structure for checking active monitors
struct ActiveMonitors
{
	std::wstring displayNum;
	std::wstring monitorPathFromTarget;
};

// Structure for saving monitor's position
struct MonitorPos {
	std::wstring monitorPathFromTarget;
	int x, y;
};

using MonitorsPositions = std::vector<MonitorPos>;
// Structure for saving layout preset with its monitor friendly names
struct LayoutPreset {
	std::wstring friendlyName;
	// for each orientation there is layout of monitors with its positions 
	std::map<int, MonitorsPositions> layoutByOrientation;
};

std::vector<MonitorInfo> listMonitors();
std::vector<ActiveMonitors> checkMonitors();

void setOrientation(const MonitorInfo& selectedMonitor, int rotate);
void saveLayoutForOrientation(int orientation);
void applyLayoutForOrientation(int orientation);
std::wstring generateMonitorConfigKey(const std::vector<ActiveMonitors>& monitors);
std::wstring keyToFriendlyMonitorNames();
void startMonitorWatcher();
namespace MonitorManager {
	extern std::atomic<int> currentOrientation;
	extern MonitorInfo selectedMonitor;
	extern std::vector<MonitorInfo> monitors;
	extern std::vector<ActiveMonitors> activeMonitors;
	extern std::wstring currentMonitorConfigKey;
	extern std::map<std::wstring, LayoutPreset> PresetByMonitorSet;
}
#endif // MONITOR_MANAGER_H