
#include "monitor_manager.h"
#include "toast_utils.h"
#include "g_flags.h"
using namespace WinToastLib;
namespace MonitorManager {
	std::vector<MonitorInfo> monitors;
	std::vector<ActiveMonitors> activeMonitors;
	std::wstring currentMonitorConfigKey;
	std::map<std::wstring, LayoutPreset> PresetByMonitorSet;
	std::atomic<int> currentOrientation = -1;
}

void startMonitorWatcher() {
	std::thread([]() {
		std::wstring lastKey;
		while (!stopMonitorWatcher) {
			auto currentMonitors = checkMonitors();
			auto key = generateMonitorConfigKey(currentMonitors);

			if (key != lastKey) {
				MonitorManager::activeMonitors = currentMonitors;
				MonitorManager::currentMonitorConfigKey = key;
				MonitorManager::monitors = listMonitors();
				// Check if selected monitor is active
				bool isNowConnected = false;
				for (const auto& m : currentMonitors) {
					if (m.monitorPathFromTarget == MonitorManager::selectedMonitor.monitorPathFromTarget) {
						isNowConnected = true;
						break;
					}
				}
				if (isNowConnected) {
					setOrientation(MonitorManager::selectedMonitor, MonitorManager::currentOrientation);
				}
				// If selected monitor disconnected
				if (wasConnected && !isNowConnected) {
					wasConnected = false;

					// Stop receiving data from arduino
					if (dataReceivingActive && hSerial != INVALID_HANDLE_VALUE) {
						StopDataReceiving();
					}
				}

				// If selected monitor connected
				if (!wasConnected && isNowConnected) {
					wasConnected = true;
					// If COM-port opened start receiving data
					if (!dataReceivingActive && hSerial != INVALID_HANDLE_VALUE && hSerial != NULL) {
						StartDataReceiving();
					}
				}
			}
			lastKey = key;
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
		}).detach();
}
// Function to get useful information about monitor to display it later
std::vector<MonitorInfo> listMonitors() {
	std::vector<MonitorInfo> monitors;
	std::vector<DISPLAYCONFIG_PATH_INFO> paths;
	std::vector<DISPLAYCONFIG_MODE_INFO> modes;
	UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;
	LONG result = ERROR_SUCCESS;

	do {
		// Determine how many path and mode structures to allocate
		UINT32 pathCount, modeCount;
		result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);
		if (result != ERROR_SUCCESS) return monitors;
		// Allocate the path and mode arrays
		paths.resize(pathCount);
		modes.resize(modeCount);
		// Get all active paths and their modes
		result = QueryDisplayConfig(flags, &pathCount, paths.data(),
			&modeCount, modes.data(), nullptr);
		// The function may have returned fewer paths/modes than estimated
		paths.resize(pathCount);
		modes.resize(modeCount);
		// It's possible that between the call to GetDisplayConfigBufferSizes and QueryDisplayConfig
		// that the display state changed, so loop on the case of ERROR_INSUFFICIENT_BUFFER.
	} while (result == ERROR_INSUFFICIENT_BUFFER);

	if (result != ERROR_SUCCESS) return monitors;

	// Find active monitors 
	DISPLAY_DEVICE displayDevice = {};
	displayDevice.cb = sizeof(DISPLAY_DEVICE);
	int j = 0;
	for (int i = 0; EnumDisplayDevices(NULL, i, &displayDevice, 0); i++) {
		// Check if monitor is active
		if (!(displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)) continue;
		j++;
		MonitorInfo monitor;
		// Set index to display it later
		monitor.index = j;
		// Get internal display number
		monitor.displayNum = displayDevice.DeviceName;
		// Get Gpu the monitors connected to
		monitor.gpuName = displayDevice.DeviceString;
		// get monitor`s Device Instance Path
		DISPLAY_DEVICE monitorDevice = {};
		monitorDevice.cb = sizeof(DISPLAY_DEVICE);
		if (EnumDisplayDevices(displayDevice.DeviceName, 0, &monitorDevice, EDD_GET_DEVICE_INTERFACE_NAME)) {
			monitor.deviceName = monitorDevice.DeviceString; // Set ordinary name for now
			monitor.monitorPathFromTarget = monitorDevice.DeviceID;
		}
		// For each active path
		for (const auto& path : paths) {
			// Find the target (monitor) friendly name
			DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
			targetName.header.adapterId = path.targetInfo.adapterId;
			targetName.header.id = path.targetInfo.id;
			targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
			targetName.header.size = sizeof(targetName);

			result = DisplayConfigGetDeviceInfo(&targetName.header);
			if (result != ERROR_SUCCESS) continue;

			// Find the adapter device name
			DISPLAYCONFIG_ADAPTER_NAME adapterName = {};
			adapterName.header.adapterId = path.targetInfo.adapterId;
			adapterName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
			adapterName.header.size = sizeof(adapterName);

			result = DisplayConfigGetDeviceInfo(&adapterName.header);
			if (result != ERROR_SUCCESS) continue;

			// Find friendly name by common matching Device Instance Path  
			std::wstring monitorPathFromTarget = targetName.monitorDevicePath;
			// if Device Instance Path matching get friendly name from monitors EDID
			if (monitorPathFromTarget.find(monitor.monitorPathFromTarget) != std::wstring::npos) {
				if (targetName.flags.friendlyNameFromEdid) {
					monitor.deviceName = targetName.monitorFriendlyDeviceName;
				}
			}
		}
		monitors.push_back(monitor);
	}
	return monitors;
}

// Simplified function for updating list of active monitors
std::vector<ActiveMonitors> checkMonitors() {
	std::vector<ActiveMonitors> activeMonitors;
	DISPLAY_DEVICE displayDevice = {};
	displayDevice.cb = sizeof(DISPLAY_DEVICE);
	// get all monitors in loop
	for (int i = 0; EnumDisplayDevices(NULL, i, &displayDevice, 0); i++) {
		// Check if monitor is active
		if (!(displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)) continue;
		ActiveMonitors monitor;
		// get internal display number
		monitor.displayNum = displayDevice.DeviceName;
		// get monitor`s Device Instance Path
		DISPLAY_DEVICE monitorDevice = {};
		monitorDevice.cb = sizeof(DISPLAY_DEVICE);
		if (EnumDisplayDevices(displayDevice.DeviceName, 0, &monitorDevice, EDD_GET_DEVICE_INTERFACE_NAME)) {
			monitor.monitorPathFromTarget = monitorDevice.DeviceID;
		}
		activeMonitors.push_back(monitor);
	}
	return activeMonitors;
}

// Function for setting orientation of selected monitor
void setOrientation(const MonitorInfo& selectedMonitor, int rotate)
{
	DEVMODE dm;
	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);
	DISPLAY_DEVICE displayDevice = {};
	displayDevice.cb = sizeof(DISPLAY_DEVICE);

	// Find selected monitor
	for (int i = 0; EnumDisplayDevices(NULL, i, &displayDevice, 0); i++)
	{
		DISPLAY_DEVICE monitorDevice = {};
		monitorDevice.cb = sizeof(DISPLAY_DEVICE);
		if (!EnumDisplayDevices(displayDevice.DeviceName, 0, &monitorDevice, EDD_GET_DEVICE_INTERFACE_NAME))
			continue;

		if (selectedMonitor.monitorPathFromTarget == monitorDevice.DeviceID)
		{
			if (EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &dm))
			{
				int currentOrientation = dm.dmDisplayOrientation;
				int newOrientation;

				// set new orientation
				switch (rotate)
				{
				case 0: newOrientation = DMDO_DEFAULT; break;
				case 90: newOrientation = DMDO_90; break;
				case 180: newOrientation = DMDO_180; break;
				case 270: newOrientation = DMDO_270; break;
				default:
					return;
				}
				// if new orientation needed to swap monitor's width and height
				if ((currentOrientation == DMDO_DEFAULT && (newOrientation == DMDO_90 || newOrientation == DMDO_270)) ||
					(currentOrientation == DMDO_90 && (newOrientation == DMDO_DEFAULT || newOrientation == DMDO_180)) ||
					(currentOrientation == DMDO_180 && (newOrientation == DMDO_90 || newOrientation == DMDO_270)) ||
					(currentOrientation == DMDO_270 && (newOrientation == DMDO_DEFAULT || newOrientation == DMDO_180)))
				{
					std::swap(dm.dmPelsWidth, dm.dmPelsHeight);
				}
				dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYORIENTATION;
				dm.dmDisplayOrientation = newOrientation;
				// Changing settings of selected monitor
				LONG result = ChangeDisplaySettingsEx(displayDevice.DeviceName, &dm, NULL, CDS_UPDATEREGISTRY, NULL);

				switch (result)
				{
				case DISP_CHANGE_SUCCESSFUL:
					OutputDebugString(L"Orientation changed successfully\n");
					// If there is multiple monitors apply saved layout for them
					if (MonitorManager::activeMonitors.size() != 1 && !presetMakingActive) {
						applyLayoutForOrientation(rotate);
					}
					break;
				case DISP_CHANGE_RESTART:
					OutputDebugString(L"Restart needed to apply changes\n");
					break;
				default:
					std::wstring errorMsg = L"Could not change orientation: " + std::to_wstring(result);
					OutputDebugString(errorMsg.c_str());
					WinToastTemplate templ(WinToastTemplate::Text02);
					templ.setTextField(L"Error", WinToastTemplate::FirstLine);
					templ.setTextField(errorMsg, WinToastTemplate::SecondLine);
					templ.setDuration(WinToastTemplate::Duration::Short);
					ToastUtils::push(templ);
					break;
				}
				return;
			}
		}
	}
}

// Function for applying specified position of monitor according to preset
void applyLayoutForOrientation(int orientation) {
	const auto& configKey = MonitorManager::currentMonitorConfigKey;
	const auto& ltPreset = MonitorManager::PresetByMonitorSet[configKey];

	auto monPosIt = ltPreset.layoutByOrientation.find(orientation);
	if (monPosIt == ltPreset.layoutByOrientation.end()) {
		// If there is no preset for given orientation send corresponding notification
		WinToastTemplate templ(WinToastTemplate::Text02);
		templ.setTextField(L"Warning: no saved layout for orientation:", WinToastTemplate::FirstLine);
		templ.setTextField(std::to_wstring(orientation), WinToastTemplate::SecondLine);
		templ.setDuration(WinToastTemplate::Duration::Short);
		templ.setExpiration(11000);
		ToastUtils::push(templ);
		return;
	}
	// Set positions for monitors according to LayoutPreset
	const MonitorsPositions& monPos = monPosIt->second;
	for (const auto& pos : monPos) {
		for (const auto& monitor : MonitorManager::activeMonitors) {
			// Get monitor by Device Instance Path
			if (monitor.monitorPathFromTarget == pos.monitorPathFromTarget) {
				DEVMODE devMode = {};
				devMode.dmSize = sizeof(DEVMODE);
				if (EnumDisplaySettings(monitor.displayNum.c_str(), ENUM_CURRENT_SETTINGS, &devMode)) {
					devMode.dmPosition.x = pos.x;
					devMode.dmPosition.y = pos.y;
					devMode.dmFields = DM_POSITION;
					ChangeDisplaySettingsEx(monitor.displayNum.c_str(), &devMode, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
				}
				break;
			}
		}
	}
	// Apply changes for all at once
	ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
}

void saveLayoutForOrientation(int orientation) {
	// Saving Layout preset for given orientation
	MonitorsPositions monPos;
	for (const auto& monitor : MonitorManager::monitors) {
		DEVMODE devMode = {};
		devMode.dmSize = sizeof(DEVMODE);
		if (EnumDisplaySettings(monitor.displayNum.c_str(), ENUM_CURRENT_SETTINGS, &devMode)) {
			monPos.push_back({
				monitor.monitorPathFromTarget,
				devMode.dmPosition.x,
				devMode.dmPosition.y
				});
		}
	}

	// Generating key for active monitors
	auto key = generateMonitorConfigKey(MonitorManager::activeMonitors);
	// Adding new Monitor Position
	LayoutPreset& ltPreset = MonitorManager::PresetByMonitorSet[key];
	ltPreset.friendlyName = keyToFriendlyMonitorNames();
	ltPreset.layoutByOrientation[orientation] = monPos;
}

// Generating key with Device Instance Path of active monitors
std::wstring generateMonitorConfigKey(const std::vector<ActiveMonitors>& monitors) {
	std::vector<std::wstring> ids;
	for (const auto& mon : monitors) {
		ids.push_back(mon.monitorPathFromTarget);
	}
	std::wstring key;
	for (const auto& id : ids) {
		if (!key.empty()) key += L"+";
		key += id;
	}
	return key;
}

std::wstring keyToFriendlyMonitorNames() {
	std::vector<std::wstring> names;
	for (const auto& active : MonitorManager::activeMonitors) {
		auto it = std::find_if(
			MonitorManager::monitors.begin(),
			MonitorManager::monitors.end(),
			[&](const MonitorInfo& info) {
				return info.monitorPathFromTarget == active.monitorPathFromTarget;
			}
		);
		if (it != MonitorManager::monitors.end()) {
			// Saving monitors'name and gpu it is connected to
			names.push_back(it->deviceName + L" (" + it->gpuName + L")");
		}
	}
	// Combining monitors names
	std::wstring result;
	for (size_t i = 0; i < names.size(); ++i) {
		if (i > 0) result += L"+";
		result += names[i];
	}
	return result;
}

